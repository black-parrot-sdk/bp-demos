#include <stdint.h>
#include <stddef.h>

volatile void *volatile_memcpy(volatile void *dst, const volatile void *src, size_t size)
{
    unsigned dst_offset = (uintptr_t)dst & 3UL;
    unsigned src_offset = (uintptr_t)src & 3UL;
    size_t remain, aligned;
    if(!(dst_offset % 4) && !(src_offset % 4)) {
        aligned = size / 4;
        remain = size % 4;
        for(size_t i = 0;i < aligned;i++) {
            ((volatile uint32_t *)dst)[i] = ((volatile uint32_t *)src)[i];
        }
        for(size_t i = 0;i < remain;i++) {
            ((volatile uint8_t *)dst)[aligned * 4 + i] = ((volatile uint8_t *)src)[aligned * 4 + i];
        }
    }
    else if(!(dst_offset % 2) == 0 && !(src_offset % 2) == 0) {
        aligned = size / 2;
        remain = size % 2;
        for(size_t i = 0;i < aligned;i++) {
            ((volatile uint16_t *)dst)[i] = ((volatile uint16_t *)src)[i];
        }
        if(remain)
            ((volatile uint8_t *)dst)[aligned * 2] = ((volatile uint8_t *)src)[aligned * 2];
    }
    else {
        for(size_t i = 0;i < size;i++)
            ((volatile uint8_t *)dst)[i] = ((volatile uint8_t *)src)[i];
    }
    return dst;
}

