#ifndef FIFO
#define FIFO

// (FIFO_SIZE - 1) must >= TCP_WND in lwip (see lwipopts.h)
#include "lwip/opt.h"
#define FIFO_SIZE (TCP_WND + 1)

struct fifo_desc {
    // Total capacity: FIFO_SIZE - 1
    char fifo[FIFO_SIZE];
    int read_idx ;
    int write_idx;
// ----------------------
// |0| | | | | | |7| ...|
// ----------------------
//  ^         ^
//  |         |
//  read_idx  write_idx (point to empty)

};

struct fifo_desc *fifo_desc_alloc();
void fifo_desc_free(struct fifo_desc *desc);
int get_fifo_free_cnt(struct fifo_desc *desc);
int fifo_push(struct fifo_desc *desc, char buf[], int buf_size);
int fifo_pop(struct fifo_desc *desc, char buf[], int buf_size);
#endif
