#include "lwip/timeouts.h"
#include "ethernet_driver.h"
#include "lwip/netif.h"

extern struct netif netif;

#define PERIODIC_TASK() do { \
    ethernet_recv(&netif); \
    sys_check_timeouts(); \
} while(0)
