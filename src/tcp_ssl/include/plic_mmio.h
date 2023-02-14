
#ifndef PLIC_MMIO
#define PLIC_MMIO

#define plic_base       0x20000000UL
#define plic_priority_reg  (plic_base + 0x4UL)
#define plic_pending_reg   (plic_base + 0x1000UL)
#define plic_enable_reg    (plic_base + 0x2000UL)
#define plic_threshold_reg (plic_base + 0x200000UL)
#define plic_cc_reg        (plic_base + 0x200004UL)

#endif
