
include Makefile.frag

RISCV_GCC       = $(CROSS_COMPILE)gcc
RISCV_GCC_OPTS  = -march=rv64imafdc -mabi=lp64d -mcmodel=medany -I $(BP_SDK_INCLUDE_DIR)
RISCV_LINK_OPTS = -T $(BP_SDK_LINKER_DIR)/riscv.ld -L$(BP_SDK_LIB_DIR) -Wl,--whole-archive -lperch -Wl,--no-whole-archive
MKLFS           = dramfs_mklfs 128 64


.PHONY: all

all: $(addsuffix .riscv, $(BP_DEMOS))

%.riscv:
	$(RISCV_GCC) -o $@ $(wildcard src/$*/*.c) $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

lfs_demo.riscv: ./src/lfs_demo/lfs.c ./src/lfs_demo/main.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

# target_input_files defined in Makefile.frag
src/%/lfs.c:
	cd $(dir $@); \
		$(MKLFS) $($(addsuffix _input_files, $*)) > $(notdir $@)

clean:
	rm -f *.riscv
	rm -f $(wildcard src/*/lfs.c)

