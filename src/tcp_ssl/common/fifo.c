#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fifo.h"


struct fifo_desc *fifo_desc_alloc()
{
    struct fifo_desc *desc = malloc(sizeof(struct fifo_desc));
    if(desc == NULL)
        return NULL;
    desc->read_idx = 0;
    desc->write_idx = 0;
    return desc;
}

void fifo_desc_free(struct fifo_desc *desc)
{
    free(desc);
}


int get_fifo_free_cnt(struct fifo_desc *desc)
{
    int total_cnt;
    int read_idx = desc->read_idx;
    int write_idx = desc->write_idx;
    if(read_idx > write_idx)
        total_cnt = read_idx - write_idx - 1;
    else
        total_cnt = read_idx + FIFO_SIZE - write_idx - 1;
    return total_cnt;
}

// return total write counts. It will not return -1.
int fifo_push(struct fifo_desc *desc, char buf[], int buf_size)
{
    int total_cnt, first_cnt, second_cnt, remain, len;
    remain = buf_size;
    int read_idx = desc->read_idx;
    int write_idx = desc->write_idx;

    total_cnt = get_fifo_free_cnt(desc);
    if(total_cnt == 0 || get_fifo_free_cnt(desc) < buf_size)
        goto done;

    if(read_idx > write_idx) {
        first_cnt = total_cnt;
    }
    else {
        first_cnt = FIFO_SIZE - write_idx;
        if(read_idx == 0)
            first_cnt -= 1;
    }

    second_cnt = (write_idx + total_cnt) - FIFO_SIZE;
    len = (buf_size <= first_cnt) ? buf_size : first_cnt;
    memcpy(&(desc->fifo[write_idx]), buf, len);
    remain = buf_size - len;

    if(remain && second_cnt > 0) {
        if(remain <= second_cnt) {
            memcpy(desc->fifo, &buf[first_cnt], remain);
            remain = 0;
        }
        else {
            memcpy(desc->fifo, &buf[first_cnt], second_cnt);
            remain -= second_cnt;
        }
    }
done:
    write_idx = (write_idx + (buf_size - remain)) % FIFO_SIZE;
    desc->write_idx = write_idx;
    return buf_size - remain;
}

static int get_fifo_used_cnt(struct fifo_desc *desc)
{
    int total_cnt;
    int read_idx = desc->read_idx;
    int write_idx = desc->write_idx;
    total_cnt = FIFO_SIZE - (read_idx - write_idx);
    if(write_idx >= read_idx) {
        total_cnt -= FIFO_SIZE;
    }
    return total_cnt;
}

// return total read counts. It will not return -1.
int fifo_pop(struct fifo_desc *desc, char buf[], int buf_size)
{
    int total_cnt, first_cnt, second_cnt, remain, len;
    int read_idx = desc->read_idx;
    int write_idx = desc->write_idx;
    remain = buf_size;

    total_cnt = get_fifo_used_cnt(desc);
    if(!total_cnt)
        goto done;

    first_cnt = FIFO_SIZE - read_idx;
    if(write_idx >= read_idx) {
        first_cnt = write_idx - read_idx;
    }

    second_cnt = (read_idx + total_cnt) - FIFO_SIZE;
    len = (buf_size <= first_cnt) ? buf_size : first_cnt;
    memcpy(buf, &(desc->fifo[read_idx]), len);
    remain = buf_size - len;

    if(remain && second_cnt > 0) {
        if(remain <= second_cnt) {
            memcpy(&buf[first_cnt], desc->fifo, remain);
            remain = 0;
        }
        else {
            memcpy(&buf[first_cnt], desc->fifo, second_cnt);
            remain -= second_cnt;
        }
    }
done:
    read_idx = (read_idx + (buf_size - remain)) % FIFO_SIZE;
    desc->read_idx = read_idx;
    return buf_size - remain;
}
