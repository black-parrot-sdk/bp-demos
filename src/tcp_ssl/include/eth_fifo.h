#ifndef ETH_FIFO
#define ETH_FIFO
//#include "lwip/pbuf.h"
void eth_fifo_init(void);
int eth_fifo_push(volatile char packet[], int packet_size);
//int eth_fifo_size_lookahead(void);
struct pbuf *eth_fifo_pop(void);
//int eth_fifo_pop(char buf[], int buf_size);
#endif
