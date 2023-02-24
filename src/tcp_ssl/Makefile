
.PHONY: all clean

BUILD_DIR ?= build
TOOL_DIR ?= ./tool
PORT ?= polling

CC := $(CROSS_COMPILE)gcc

all: $(BUILD_DIR)/ssl_server.nbf $(BUILD_DIR)/ssl_client.nbf \
    $(BUILD_DIR)/tcp_server.nbf $(BUILD_DIR)/tcp_client.nbf

COMMON_SRC         := ./common/ethernet_driver/ethernet_driver.c    \
                      ./common/volatile_memcpy.c                    \
                      ./common/fifo.c                               \
                      $(wildcard common/ethernet_driver/$(PORT)/*.c)

LWIP_SERVER_SRC    := $(COMMON_SRC) ./common/lwip_server.c ./tcp/tcp_server.c
LWIP_CLIENT_SRC    := $(COMMON_SRC) ./common/lwip_client.c ./tcp/tcp_client.c
TIME_SRC           := ./common/time.c
WOLFSSL_SERVER_SRC := $(COMMON_SRC) ./common/lwip_server.c ./ssl/ssl_server.c
WOLFSSL_CLIENT_SRC := $(COMMON_SRC) ./common/lwip_client.c ./ssl/ssl_client.c

CFLAGS     := -mabi=lp64 -march=rv64imafd -mcmodel=medany -Os \
        -Wno-unused-parameter -I./include  \
        -Wno-unused-function -I$(BP_SDK_INCLUDE_DIR) \
        -Wall -Wextra -static -std=gnu99
LINK_OPTS  := -T$(BP_SDK_LINKER_DIR)/riscv.ld  -L$(BP_SDK_LIB_DIR) \
        -Wl,--whole-archive -lperch -Wl,--no-whole-archive -lperch -llwip -lm

# Add -DENABLE_DHCP to CFLAGS if you want the DHCP client feature.
CFLAGS += -DENABLE_DHCP

build_certificates:
	make -C $(TOOL_DIR) all

tcp_server.riscv: $(LWIP_SERVER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LWIP_SERVER_SRC) $(LINK_OPTS) -o $(BUILD_DIR)/$@

tcp_client.riscv: $(LWIP_CLIENT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LWIP_CLIENT_SRC) $(LINK_OPTS) -o $(BUILD_DIR)/$@

ssl_server.riscv: $(WOLFSSL_SERVER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(WOLFSSL_SERVER_SRC) -lwolfssl $(TIME_SRC) $(LINK_OPTS) -o $(BUILD_DIR)/$@

ssl_client.riscv: $(WOLFSSL_CLIENT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(WOLFSSL_CLIENT_SRC) -lwolfssl $(TIME_SRC) $(LINK_OPTS) -o $(BUILD_DIR)/$@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
