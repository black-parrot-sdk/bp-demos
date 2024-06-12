#include <assert.h>
#include <string.h>
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/mem.h"
#include "fifo.h"
#include "lwip/tcpbase.h"
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/openssl/ssl.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/memory.h>
#define USE_CERT_BUFFERS_2048
#include "my_certs_test.h"
#include "tcp_server_task.h"
#include "lwip_common.h"

#define SSL_PROGRESS_ERROR  0
#define SSL_PROGRESS_ACCEPT 1
#define SSL_PROGRESS_IO     2

#define MSG_SIZE 1024
static char mesg[MSG_SIZE + 1];

#define assertf(cond, ...) do { \
    if(!(cond)) { \
        printf("server: " __VA_ARGS__); \
        assert(cond); \
    } \
} while(0)

static int tcp_server_task_create(struct tcp_pcb *newpcb);
static int tcp_server_task_close(int task_id);
static int tcp_server_task_data_input(int task_id, struct pbuf *p);
static void tcp_server_task_run_all(void);

struct tcp_server_task tcp_server_task = {
    .task_create = tcp_server_task_create,
    .task_scheduler = tcp_server_task_run_all,
    .task_data_input = tcp_server_task_data_input,
    .task_close = tcp_server_task_close
};


struct ssl_desc {
    WOLFSSL *ssl;
    struct fifo_desc *fifo_desc;
    int progress;
};

struct tcp_server_task_struct {
    struct ssl_desc data;
    struct tcp_pcb *pcb;
    int task_id;
    int in_use;
};

static int ssl_media_send(WOLFSSL *ssl, char *buf, int sz, void *ctx);
static int ssl_media_recv(WOLFSSL *ssl, char *buf, int sz, void *ctx);

// Configurable upper limit.
#define MAX_SSL_CONNECTION_COUNT 8
static WOLFSSL_CTX *ctx = NULL;

static struct tcp_server_task_struct tcp_server_task_array[MAX_SSL_CONNECTION_COUNT];

static void wolfssl_log_callback(const int logLevel, const char *const logMessage)
{
    // ERROR_LOG, INFO_LOG, ENTER_LOG, LEAVE_LOG, OTHER_LOG
//    if(logLevel == ERROR_LOG) {
//        printf("%s\n", logMessage);
//    }
}
/*
#define MEM_LIBC_STATSHELPER_SIZE LWIP_MEM_ALIGN_SIZE(sizeof(mem_size_t))

// Using malloc from lwIP. Only work when MEM_LIBC_MALLOC == 1
static void *custom_malloc(size_t size)
{
    return mem_malloc(size);
}
// Using free from lwIP. Only work when MEM_LIBC_MALLOC == 1
static void custom_free(void *ptr)
{
    mem_free(ptr);
}
// Using malloc/free from lwIP. Only work when MEM_LIBC_MALLOC == 1
static void *custom_realloc(void *ptr, size_t size)
{
    size_t old_size;
    void *size_ptr, *ret = NULL;
    if(ptr == NULL) {
        ret = custom_malloc(size);
    }
    else {
        size_ptr = (uint8_t *)ptr - MEM_LIBC_STATSHELPER_SIZE;
        old_size = *(mem_size_t *)size_ptr;
        if(old_size >= size)
            ret = mem_trim(ptr, size);
        else {
            ret = mem_malloc(size);
            if(ret != NULL) {
                memcpy(ret, ptr, old_size);
                mem_free(ptr);
            }
        }
    }
    return ret;
}*/

// wolfssl global init
static void tcp_server_task_global_init()
{
    int err = wolfSSL_Init();
    assertf(err == WOLFSSL_SUCCESS, "wolfSSL_Init() failed with error code %d\n", err);
    printf("wolfssl init success\n");

    wolfSSL_SetLoggingCb(wolfssl_log_callback);
    wolfSSL_Debugging_ON();

    // choose protocol
    WOLFSSL_METHOD *method = wolfTLSv1_2_server_method();
    ctx    = wolfSSL_CTX_new(method);
    assertf(ctx != NULL, "wolfSSL_CTX_new() failed\n");
    wolfSSL_CTX_SetIORecv(ctx, ssl_media_recv);
    wolfSSL_CTX_SetIOSend(ctx, ssl_media_send);

    err = wolfSSL_CTX_use_certificate_buffer(ctx, server_cert_pem, \
            sizeof(server_cert_pem), WOLFSSL_FILETYPE_PEM);
    assertf(err == WOLFSSL_SUCCESS, "Cannot load server certificate\n");

    err = wolfSSL_CTX_use_PrivateKey_buffer(ctx, server_private_key_pem, \
            sizeof(server_private_key_pem), WOLFSSL_FILETYPE_PEM);
    assertf(err == WOLFSSL_SUCCESS, "Cannot load server private key\n");

    // set custom malloc/realloc/free for wolfSSL (to leverage the debug stats in lwIP)
//    wolfSSL_SetAllocators(custom_malloc, custom_free, custom_realloc);
}


// return task_id; return -1 on error
static int tcp_server_task_create(struct tcp_pcb *newpcb)
{
    static int tcp_server_task_global_inited = 0;
    struct tcp_server_task_struct *allocated_task;
    int ret = -1;
    WOLFSSL *ssl = NULL;
    if(!newpcb) {
        return ret;
    }
    if(tcp_server_task_global_inited == 0) {
        tcp_server_task_global_init();
        tcp_server_task_global_inited = 1;
    }

    if(!(ssl = wolfSSL_new(ctx)))
        return ret;

    for(int i = 0;i < MAX_SSL_CONNECTION_COUNT;i++) {
        if(tcp_server_task_array[i].in_use == 0) {
            struct fifo_desc *desc = fifo_desc_alloc();
            allocated_task = &tcp_server_task_array[i];
            if(desc) {
                allocated_task->data.fifo_desc = desc;
                allocated_task->in_use = 1;
                allocated_task->data.ssl = ssl;
                allocated_task->pcb = newpcb;
                wolfSSL_set_fd(ssl, i);
                allocated_task->data.progress = SSL_PROGRESS_ACCEPT;
                ret = i;
                break;
            }
            else {
                return ret;
            }
        }
    }
    return ret;
}

static int tcp_server_task_close(int task_id)
{
    struct tcp_server_task_struct *task = &tcp_server_task_array[task_id];
    WOLFSSL *ssl;
    if(task_id >= 0 && task_id < MAX_SSL_CONNECTION_COUNT) {
        if(task->in_use) {
            task->in_use = 0;
            fifo_desc_free(task->data.fifo_desc);

            ssl = task->data.ssl;
            wolfSSL_free(ssl);
            return 0;
        }
    }
    return -1;
}

static void tcp_server_task_run(int task_id)
{
    WOLFSSL *ssl = tcp_server_task_array[task_id].data.ssl;
    int *progress = &(tcp_server_task_array[task_id].data.progress);
    int ret, read_count;
    switch(*progress) {
        case SSL_PROGRESS_ACCEPT:
            ret = wolfSSL_accept(ssl);
            ret = wolfSSL_get_error(ssl, ret);
            if(ret == WOLFSSL_ERROR_NONE) {
                printf("wolfSSL_accept() success\n");
                *progress = SSL_PROGRESS_IO;
            }
            else if(ret != WOLFSSL_ERROR_WANT_READ && ret != WOLFSSL_ERROR_WANT_WRITE) {
                assertf(0, "wolfSSL_accept() failed with error code %d\n", ret);
                *progress = SSL_PROGRESS_ERROR;
            }
            break;
        case SSL_PROGRESS_IO:
            read_count = wolfSSL_read(ssl, mesg, sizeof(mesg) - 1);
            ret = wolfSSL_get_error(ssl, read_count);
            if(ret == WOLFSSL_ERROR_NONE) {
                mesg[read_count] = '\0';
                printf("received mesg: %s\n", mesg);
            }
            else if(ret != WOLFSSL_ERROR_WANT_READ && ret != WOLFSSL_ERROR_WANT_WRITE) {
                printf("wolfSSL_read() failed with error code %d\n", ret);
                *progress = SSL_PROGRESS_ERROR;
                assertf(!tcp_server_task_close(task_id), "tcp_server_task_close() failed\n");
            }
            break;
        case SSL_PROGRESS_ERROR:
            // an error has occurred
            break;
    }
}
static void tcp_server_task_run_all(void)
{
    for(int i = 0;i < MAX_SSL_CONNECTION_COUNT;i++) {
        if(tcp_server_task_array[i].in_use) {
            tcp_server_task_run(i);
        }
    }
}

/* return 0 if entire data is put into fifo */
// lwip pushes incoming packets to fifo through this function
static int tcp_server_task_data_input(int task_id, struct pbuf *p)
{
    if(!p) {
        printf("tcp_server_task_data_input: NULL pbuf\n");
        return 0;
    }
    if(task_id < 0 || task_id >= MAX_SSL_CONNECTION_COUNT) {
        printf("tcp_server_task_data_input: invalid task_id: %d\n", task_id);
        return -1;
    }
    struct fifo_desc *desc = tcp_server_task_array[task_id].data.fifo_desc;
    if(get_fifo_free_cnt(desc) >= p->tot_len) {
        struct pbuf *ptr = p;
        while(1) {
            fifo_push(desc, ptr->payload, ptr->len);
            if(ptr->len == ptr->tot_len)
                break;
            ptr = ptr->next;
        }
        return 0;
    }
    return -1;
}

/* Blocking Send */
// wolfssl sends out packets through this function
// Seems like if ssl_media_send cannot send out everying all at once (non-blocking),
// wolfSSL_write will return WANT_WRITE even if some data has aleady been sent out
// to lwIP.
static int ssl_media_send(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    (void)ssl;
    (void)ctx;
    int ret, free, len;
    int task_id = wolfSSL_get_fd(ssl);
    int sum = 0, remain = sz;
    struct tcp_pcb *tpcb = tcp_server_task_array[task_id].pcb;
    while(remain) {
        PERIODIC_TASK();
        free = tcp_sndbuf(tpcb);
        len = (remain < free) ? remain : free;
        if(len != 0) {
            ret = tcp_write(tpcb, &buf[sum], len, TCP_WRITE_FLAG_COPY);
            assert(ret == ERR_OK);
            sum += len;
            remain -= len;
        }
    }
    return sz;
}


/* Non-blocking Recv
*   wolfssl pops and reads the packets previously pushed by media_fifo_push
* through this function
*/
static int ssl_media_recv(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    (void)ctx;
    int ret = WOLFSSL_CBIO_ERR_GENERAL;
    int task_id = wolfSSL_get_fd(ssl);
    struct fifo_desc *desc = tcp_server_task_array[task_id].data.fifo_desc;
    ret = fifo_pop(desc, buf, sz);
    if(ret == 0) {
        // Would block
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }
    return ret;
}
