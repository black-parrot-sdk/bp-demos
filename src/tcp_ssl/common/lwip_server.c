#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
#include "lwip/dhcp.h"
#include "lwip/stats.h"

#include "ethernet_driver.h"
#include "tcp_server_task.h"
#include "lwip_common.h"

// Server configuration:
#ifndef ENABLE_DHCP
#define SERVER_IP "127.0.0.1"
#define SERVER_MASK "255.0.0.0"
#endif
#define SERVER_PORT 12345
#define SERVER_MAC "\x22\x33\x44\x55\x66\x77"

#define assertf(cond, ...) do { \
    if(!(cond)) { \
        printf("server: " __VA_ARGS__); \
        assert(cond); \
    } \
} while(0)

struct netif netif;

void tcp_err_callback(void *arg, err_t err)
{
    (void)arg;
    assertf(0, "tcp_err_callback() called with error code %d\n", err);
}
static err_t ethif_init(struct netif *netif)
{
    const char hwaddr[6] = SERVER_MAC;
    const int hwaddr_len = sizeof(hwaddr) / sizeof(*hwaddr);
    netif->state = NULL;
    netif->hwaddr_len = hwaddr_len;
    memcpy(netif->hwaddr, hwaddr, hwaddr_len);
    netif->mtu = 1500;
    memcpy(netif->name, "en", 2);
    netif->output = etharp_output;
    netif->linkoutput = ethernet_send;
    netif->flags |= (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP);
    ethernet_init(netif);
    return 0;
}

// when ethernet frame sending into lwip: call netif->input()
// when ethernet frame sending out  lwip: call netif->linkoutput()

// It seems to me the size of p can be as large as TCP_WND, and there isn't a way to
// receive the data partially. Therefore the upper layer should have a buffer with
// size at least TCP_WND.
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    int retval, task_id;
    retval = err;
    task_id = (intptr_t)arg;
    if(retval == ERR_OK) {
        if(p) {
            if(tcp_server_task.task_data_input(task_id, p) == -1) {
                // not enough space for entire pbuf
                retval = ERR_MEM;
            }
            else {
                // successfully put the data into FIFO
                tcp_recved(tpcb, p->tot_len);
            }
        }
        else {
            printf("Client closed\n");
            tcp_close(tpcb);
            retval = ERR_OK;
            // Close user connection:
            tcp_server_task.task_close(task_id);
//            stats_display();
        }
    }
/* If the callback function returns ERR_OK or ERR_ABRT it must have freed the pbuf */
    if(p && (retval == ERR_OK || retval == ERR_ABRT)) {
        pbuf_free(p);
    }
    return retval;
}
err_t tcp_accept_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    (void)arg;
    int task_id;
    assertf(err == ERR_OK, "tcp_accept_callback() called with error code %d\n", err);

    printf("server: tcp accept success\n");
    // set callback functions
    tcp_recv(tpcb, tcp_recv_callback);
    tcp_err(tpcb, tcp_err_callback);
    if((task_id = tcp_server_task.task_create(tpcb)) < 0) {
        tcp_abort(tpcb);
        // tpcb is now deallocated.
        return ERR_ABRT;

    }
    tcp_arg(tpcb, (void *)(intptr_t)task_id);
    return ERR_OK;
}

int main()
{
    struct tcp_pcb *pcb = NULL;
    ip4_addr_t ipaddr;
    lwip_init();
    err_t err;
    // Add new network interface to lwip
#ifdef ENABLE_DHCP
    netif_add(&netif, NULL, NULL, NULL, NULL, ethif_init, netif_input);
#else
    ipaddr.addr = ipaddr_addr(SERVER_IP);
    ip4_addr_t netmask = {ipaddr_addr(SERVER_MASK)};

    netif_add(&netif, &ipaddr, &netmask, NULL, NULL, ethif_init, netif_input);
#endif

    netif_set_up(&netif);

#ifdef ENABLE_DHCP
    err = dhcp_start(&netif);
    assertf(err == ERR_OK, "dhcp_start() failed with error code %d\n", err);

    printf("Waiting for DHCP completion\n");
    while(dhcp_supplied_address(&netif) == 0) {
        PERIODIC_TASK();
    }
    printf("DHCP success: ");
    for(int i = 0;i < 4;i++) {
        uint32_t addr = netif.ip_addr.addr;
        addr = (addr >> (i * 8)) & 255U;
        printf("%d ", addr);
    }
    printf("\n");
#endif

    pcb = tcp_new();
    assertf(pcb != NULL, "tcp_new() failed\n");

    ipaddr.addr = netif.ip_addr.addr;
    err = tcp_bind(pcb, &ipaddr, SERVER_PORT);
    assertf(err == ERR_OK, "tcp_bind() failed with error code %d\n", err);

    // set TCP_LISTEN_BACKLOG=1 in lwipopts.h to use backlog:
    pcb = tcp_listen(pcb);
    assertf(pcb != NULL, "tcp_listen() falied\n");
    // after tcp_listen, pcb is deallocated

    // set callback function(s)
    tcp_accept(pcb, tcp_accept_callback);
//    stats_display();
    printf("Entering main loop\n");
    while(1) {
        PERIODIC_TASK();
        tcp_server_task.task_scheduler();
    }
    return 0;
}


