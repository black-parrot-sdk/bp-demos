#ifndef TCP_TASK_H
#define TCP_TASK_H

#include "lwip/pbuf.h"

struct tcp_client_task {
    int (*task_create) (struct tcp_pcb *newpcb);
    void (*task_scheduler)(void);
    int  (*task_data_input)(int task_id, struct pbuf *p);
    int  (*task_close)(int task_id);
};

// User defined variable
extern struct tcp_client_task tcp_client_task;

#endif
