#include <assert.h>
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/tcpbase.h"
#include "fifo.h"
#include "tcp_server_task.h"

#define MSG_SIZE 1024
static char mesg[MSG_SIZE + 1];

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

struct simple_desc {
    struct fifo_desc *fifo_desc;
};

struct tcp_server_task_struct {
    struct simple_desc data;
    struct tcp_pcb *pcb;
    int task_id;
    int in_use;
};


// Configurable upper limit.
#define MAX_TCP_CONNECTION_COUNT 8

static struct tcp_server_task_struct tcp_server_task_array[MAX_TCP_CONNECTION_COUNT];

// return task_id; return -1 on error
static int tcp_server_task_create(struct tcp_pcb *newpcb)
{
    struct tcp_server_task_struct *allocated_task;
    int ret = -1;
    if(!newpcb) {
        return ret;
    }

    for(int i = 0;i < MAX_TCP_CONNECTION_COUNT;i++) {
        if(tcp_server_task_array[i].in_use == 0) {
            struct fifo_desc *desc = fifo_desc_alloc();
            allocated_task = &tcp_server_task_array[i];
            if(desc) {
                allocated_task->data.fifo_desc = desc;
                allocated_task->in_use = 1;
                allocated_task->pcb = newpcb;
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
    if(task_id >= 0 && task_id < MAX_TCP_CONNECTION_COUNT) {
        if(task->in_use) {
            task->in_use = 0;
            fifo_desc_free(task->data.fifo_desc);
            return 0;
        }
    }
    return -1;
}

static void tcp_server_task_run(int task_id)
{
    struct fifo_desc *desc = tcp_server_task_array[task_id].data.fifo_desc;
    int ret = fifo_pop(desc, mesg, sizeof(mesg) - 1);
    if(ret == 0) {
        // Would block
        return;
    }
    mesg[ret] = '\0';
    printf("%s\n", mesg);
}
static void tcp_server_task_run_all(void)
{
    for(int i = 0;i < MAX_TCP_CONNECTION_COUNT;i++) {
        if(tcp_server_task_array[i].in_use) {
            tcp_server_task_run(i);
        }
    }
}

/* return 0 if entire data is put into fifo */
// lwip pushes incoming packets to fifo through this function
static int tcp_server_task_data_input(int task_id, struct pbuf *p)
{
    if(task_id < 0 || task_id >= MAX_TCP_CONNECTION_COUNT) {
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
