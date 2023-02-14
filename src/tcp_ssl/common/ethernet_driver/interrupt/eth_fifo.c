#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "eth_fifo.h"
#include "volatile_memcpy.h"
#include "lwip/lwipopts.h"
#include "lwip/pbuf.h"

// TCP_MSS + TCP header size + IP header size + Ethernet header size
#define MAX_PACKET_SIZE (TCP_MSS + 20 + 20 + 14)
#define ETH_FIFO_SLOT 8

static struct pbuf * volatile eth_fifo[ETH_FIFO_SLOT];

// NOTE: the reader and write should sit in the same core
static volatile int read_ptr  = 0; // read-only for writer; rw for reader
static volatile int write_ptr = 0; // read-only for reader; rw for writer

void eth_fifo_init()
{
    struct pbuf *p;
    for(int i = 0;i < ETH_FIFO_SLOT;i++) {
        p = pbuf_alloc(PBUF_RAW, MAX_PACKET_SIZE, PBUF_RAM);
        if(p == NULL) {
            for(;;){}
        }
        eth_fifo[i] = p;
    }
}

static int eth_fifo_not_full()
{
    return ((write_ptr + 1) % ETH_FIFO_SLOT) != read_ptr;
}
static int eth_fifo_is_empty()
{
    return read_ptr == write_ptr;
}

// return value: send count in byte
int eth_fifo_push(volatile char packet[], int packet_size)
{
    int count = 0;
    struct pbuf *p;
    if(packet_size <= 0 || packet_size > MAX_PACKET_SIZE) {
        return 0;
    }
    if(eth_fifo_not_full()) {
        p = eth_fifo[write_ptr];
        // assuming payload is contiguous
        volatile_memcpy(p->payload, packet, packet_size);
        write_ptr = (write_ptr + 1) % ETH_FIFO_SLOT;
        count = packet_size;
    }
    else {
        // fifo is full
    }
    return count;
}

struct pbuf *eth_fifo_pop()
{
    struct pbuf *p = NULL, *new_p;
    if(!eth_fifo_is_empty()) {
        p = eth_fifo[read_ptr];
        // Allocate from mem_malloc. This ensures the allocated memory is contiguous.
        new_p = pbuf_alloc(PBUF_RAW, MAX_PACKET_SIZE, PBUF_RAM);
        if(new_p == NULL)
            for(;;){}
        eth_fifo[read_ptr] = new_p;
        read_ptr = (read_ptr + 1) % ETH_FIFO_SLOT;
    }
    return p;
}


