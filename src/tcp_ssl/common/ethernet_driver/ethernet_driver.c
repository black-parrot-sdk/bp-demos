
#include <stdio.h>
#include <string.h>
#include "bp_utils.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "ethernet_driver.h"
#include "ethernet_driver_port.h"

void ethernet_init(struct netif *netif)
{
    _ethernet_init(netif);
    netif_set_link_up(netif);
}

void ethernet_recv(struct netif *netif)
{
    struct pbuf *pbuf;
    // check packet arrival
    pbuf = _ethernet_recv();
    if(pbuf) {
        if(netif->input(pbuf, netif) != ERR_OK) {
//            bp_print_string("netif input failed\n");
        }
    }
}
err_t ethernet_send(struct netif *netif, struct pbuf *p)
{
	(void)netif;
    int ret;
    if((ret = _ethernet_send(p)) == p->tot_len) {
        /* packet sent */
    }
    else {
    }
	return ERR_OK;
}
