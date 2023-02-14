#ifndef ETHERNET_DRIVER_PORT_H
#define ETHERNET_DRIVER_PORT_H

#include "lwip/netif.h"
#include "lwip/pbuf.h"

void _ethernet_init(struct netif *netif);
struct pbuf *_ethernet_recv(void);
int _ethernet_send(struct pbuf *pbuf);

#endif
