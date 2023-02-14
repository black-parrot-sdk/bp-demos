
#include <stdio.h>
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/lwipopts.h"
#include "ethernet_driver_port.h"
#include "ethernet_mmio.h"
#include "plic_mmio.h"
#include "volatile_memcpy.h"

// TCP_MSS + TCP header size + IP header size + Ethernet header size
#define MAX_PACKET_SIZE (TCP_MSS + 20 + 20 + 14)

void _ethernet_init(struct netif *netif)
{
}

struct pbuf *_ethernet_recv(void)
{
    struct pbuf *pbuf = NULL;
    unsigned packet_size;
    if(*(volatile unsigned *)eth_rx_pending_reg) {
        // packet arrival
        packet_size = *(volatile unsigned *)eth_rx_packet_size_reg;
        if(packet_size <= MAX_PACKET_SIZE) {
            pbuf = pbuf_alloc(PBUF_RAW, packet_size, PBUF_RAM);
            if(pbuf != NULL) {
                volatile_memcpy(pbuf->payload, (volatile void *)eth_rx_packet_reg, pbuf->tot_len);
            }
        }
        *(volatile unsigned *)eth_rx_pending_reg = 1;
    }
    return pbuf;
}

int _ethernet_send(struct pbuf *pbuf)
{
    // TODO: Check if TX is available
    struct pbuf *p = pbuf;
    int send_size = p->tot_len;
    int sum = 0;
    while(sum != send_size) {
        volatile_memcpy((volatile void *)eth_tx_packet_reg + sum, p->payload, p->len);
        sum += p->len;
        p = p->next;
    }

	*(volatile unsigned *)eth_tx_size_reg = send_size;
	*(volatile unsigned *)eth_tx_send_reg = 1;
    return send_size;
}
