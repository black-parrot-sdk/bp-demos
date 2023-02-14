
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "bp_trap.h"
#include "bp_utils.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "eth_fifo.h"
#include "ethernet_driver_port.h"
#include "ethernet_mmio.h"
#include "plic_mmio.h"
#include "volatile_memcpy.h"

#define ETH_INT_ID 1

static void print_str(const char mesg[])
{
    int i = 0;
    while(mesg[i])
      *(volatile char *)0x101000UL = mesg[i++];
}
static void print_num(uint32_t val)
{
    for(int i = 0;i < 8;i++) {
        uint32_t digit = val >> 28;
        if(digit < 10)
            *(volatile char *)0x101000UL = '0' + digit;
        else
            *(volatile char *)0x101000UL = 'a' + digit - 10;
        val <<= 4;
    }
}

static void ethernet_interrupt_handler(uint64_t *regs, uint64_t mcause, uint64_t instr)
{
    (void)regs;
    (void)mcause;
    (void)instr;
    // S-mode external interrupt handling
    // PLIC claim
    int plic_claim_id = *(volatile char *)plic_cc_reg;
    if(plic_claim_id == ETH_INT_ID) {
        // check if we really get RX pending
        if(*(volatile int *)eth_rx_pending_reg) {
            int rx_packet_size;
            // read out packet
            rx_packet_size = *(volatile unsigned *)(eth_rx_packet_size_reg);
            eth_fifo_push((volatile char *)eth_rx_packet_reg, rx_packet_size);

            // write 1 to clear Ethernet interrupt pending bit
            *(volatile int *)eth_rx_pending_reg = 1;
            // PLIC complete
            *(volatile int *)plic_cc_reg = plic_claim_id;
        }
        else {
            print_str("Empty interrupt\n");
        }
    }
    else {
        print_str("Unknown PLIC claim id: ");
        print_num(plic_claim_id);
        for(;;){}
    }
}


static void s_external_interrupt_enable()
{
	unsigned long tmp;
	asm volatile ("li %0, (1 << 9)\n" : "=r"(tmp));
	asm volatile ("csrs mie, %0\n" : : "r"(tmp) : "memory");
}

static void global_m_interrupt_enable()
{
	unsigned long tmp;
	asm volatile ("li %0, (1 << 3)\n" : "=r"(tmp));
	asm volatile ("csrs mstatus, %0\n" : : "r"(tmp) : "memory");
}

void _ethernet_init(struct netif *netif)
{
    // register S-mode external interrupt handler
    if(register_trap_handler(&ethernet_interrupt_handler, (9UL | 1UL << 63))) {
        print_str("Fail to register trap_handler\n");
        bp_finish(-1);
    }
    eth_fifo_init();
    // set PLIC priority
    *(volatile unsigned *)(plic_priority_reg) = 1;
    // set PLIC threshold
    *(volatile unsigned *)(plic_threshold_reg) = 0;
    // set PLIC interrupt enable
    *(volatile unsigned *)(plic_enable_reg) = 2; // 2'b10; PLIC does not use src 0
    // set Ethernet interrupt enable bit
    *(volatile int *)(eth_rx_enable_reg) = 1;
    // set BP S-mode interrupt enable bit
    s_external_interrupt_enable();
    // set BP global interrupt enable bit for M-mode
    global_m_interrupt_enable();
}
struct pbuf *_ethernet_recv(void)
{
    return eth_fifo_pop();
}

int _ethernet_send(struct pbuf *pbuf)
{
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
