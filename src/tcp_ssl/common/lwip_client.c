#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip_common.h"

// Client configuration:
#ifndef ENABLE_DHCP
#define CLIENT_IP "127.0.0.2"
#define CLIENT_MASK "255.0.0.0"
#define SERVER_IP "127.0.0.1"
#else
#define SERVER_IP ""
#endif
#define CLIENT_MAC "\x00\x11\x22\x33\x44\x55"

// Server configuration:
#define SERVER_PORT 12345

#include "tcp_client_task.h"
#include "ethernet_driver.h"

#define assertf(cond, ...) do { \
    if(!(cond)) { \
        printf("client: " __VA_ARGS__); \
        assert(cond); \
    } \
} while(0)


struct netif netif;

static err_t ethif_init(struct netif *netif)
{
    const char hwaddr[6] = CLIENT_MAC;
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
            if(tcp_client_task.task_data_input(task_id, p) == -1) {
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
            tcp_client_task.task_close(task_id);
        }
    }
/* If the callback function returns ERR_OK or ERR_ABRT it must have freed the pbuf */
    if(p && (retval == ERR_OK || retval == ERR_ABRT)) {
        pbuf_free(p);
    }
    return retval;
}
static void tcp_err_callback(void *arg, err_t err)
{
    (void)arg;
    assertf(0, "tcp_err_callback() called with error code %d\n", err);
}

err_t tcp_connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    (void)arg;
    int task_id;
    assertf(err == ERR_OK, "tcp_connect_callback() called with error code %d\n", err);

    printf("client: tcp connect success\n");
    tcp_recv(tpcb, tcp_recv_callback);
    tcp_err(tpcb, tcp_err_callback);
    if((task_id = tcp_client_task.task_create(tpcb)) < 0) {
        tcp_abort(tpcb);
        // tpcb is now deallocated.
        return ERR_ABRT;
    }
    tcp_arg(tpcb, (void *)(intptr_t)task_id);
    return ERR_OK;
}



int main()
{
 // client
    struct tcp_pcb *pcb = NULL;
    lwip_init();

#ifdef ENABLE_DHCP
    netif_add(&netif, NULL, NULL, NULL, NULL, ethif_init, netif_input);
#else
    ip4_addr_t client_ipaddr  = {ipaddr_addr(CLIENT_IP)};
    ip4_addr_t client_netmask = {ipaddr_addr(CLIENT_MASK)};
    netif_add(&netif, &client_ipaddr, &client_netmask, NULL, NULL, ethif_init, netif_input);
#endif
    ip4_addr_t server_ipaddr  = {ipaddr_addr(SERVER_IP)};
    netif_set_up(&netif);

#ifdef ENABLE_DHCP
    err_t err = dhcp_start(&netif);
    assertf(err == ERR_OK, "dhcp_start() failed with error code %d\n", err);

    printf("Entering DHCP loop\n");
    while(dhcp_supplied_address(&netif) == 0) {
        PERIODIC_TASK();
    }
    printf("DHCP success\n");
#endif

    pcb = tcp_new();
    assertf(pcb != NULL, "tcp_new() failed\n");

    // set callbacks
    tcp_err(pcb, tcp_err_callback);
    tcp_connect(pcb, &server_ipaddr, SERVER_PORT, tcp_connect_callback);
    printf("Entering main loop\n");
    while(1) {
        PERIODIC_TASK();
        tcp_client_task.task_scheduler();
    }
    return 0;
}
