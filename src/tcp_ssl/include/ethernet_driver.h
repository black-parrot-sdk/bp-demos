#ifndef ETHERNET_DRIVER_H
#define ETHERNET_DRIVER_H
#include "lwip/netif.h"
#include "lwip/pbuf.h"
void ethernet_init(struct netif *netif);
void ethernet_recv(struct netif *netif);
err_t ethernet_send(struct netif *netif, struct pbuf *p);
#endif
