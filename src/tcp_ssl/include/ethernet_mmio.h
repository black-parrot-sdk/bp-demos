
#ifndef ETHERNET_MMIO
#define ETHERNET_MMIO

#define eth_base           0x10000000UL
#define eth_rx_packet_reg      (eth_base + 0x0UL)
#define eth_tx_packet_reg      (eth_base + 0x0800UL)
#define eth_rx_packet_size_reg (eth_base + 0x1004UL)
#define eth_rx_pending_reg     (eth_base + 0x1010UL)
#define eth_rx_enable_reg      (eth_base + 0x1014UL)
#define eth_tx_send_reg        (eth_base + 0x1018UL)
#define eth_tx_size_reg        (eth_base + 0x1028UL)

#endif
