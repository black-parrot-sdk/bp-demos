
// This test is only relevant for a very specific version of zynq-parrot with a scratchpad
// The memory location may need to change.
// Use with caution (or at least low expectations of working out of box)

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bp_utils.h"

int main(int arc, char** argv) {
    volatile int * SCRATCHPAD = (volatile int *) 0x1000000;

    *(SCRATCHPAD) = 0xacbd;
    int x = *(SCRATCHPAD);
    printf("%x\n", x);

    return 0;
}

