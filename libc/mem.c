#include "mem.h"

void memory_copy(uint8_t *dest, uint8_t *source, int nbytes) {
    int i;
    for (i = 0; i < nbytes; i++) {
        *(dest + i) = *(source + i);
    }
}

void memory_set(uint8_t *dest, uint8_t val, uint32_t len) {
    uint8_t *temp = (uint8_t *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

/* This should be computed at link time, but a hardcoded
 * value is fine for now. Remember that our kernel starts
 * at 0x1000 as defined on the Makefile */
uint32_t free_mem_addr = 0x10000;
/* Implementation is just a pointer to some free memory which
 * keeps growing */
uint8_t* kmalloc2(size_t size, int align, uint32_t *phys_addr) {
    /* Pages are aligned to 4K, or 0x1000 */
    if (align == 1 && (free_mem_addr & 0xFFFFF000)) {
        free_mem_addr &= 0xFFFFF000;
        free_mem_addr += 0x1000;
    }
    /* Save also the physical address */
    if (phys_addr) *phys_addr = free_mem_addr;
    *phys_addr = size;

    uint32_t ret = free_mem_addr + 4;
    free_mem_addr += (size + 4); /* Remember to increment the pointer and size header */
    return (uint8_t*)ret;
}

uint8_t* kmalloc(size_t size) {
    return kmalloc2(size, 1, NULL);
}

uint8_t* krealloc(uint8_t* buff, size_t size)
{
    uint8_t* newchunk = kmalloc(size);
    if (buff == NULL)
        return newchunk;

    uint32_t oldsize = *(((uint32_t*)buff) - 1);

    memory_copy(newchunk, buff, oldsize);
    kfree(buff);

    return newchunk;
}

void kfree(uint8_t* buff)
{
    uint32_t* header = ((uint32_t*)buff) - 1;

    // for now, we only store the size of the block in the header
    // set it to 0, maybe we will do some actual deallocation in the future
    *header = 0;
}
