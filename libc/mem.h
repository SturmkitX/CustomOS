#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void memory_copy(uint8_t *dest, uint8_t *source, int nbytes);
void memory_set(uint8_t *dest, uint8_t val, uint32_t len);

/* At this stage there is no 'free' implemented. */
uint8_t* kmalloc2(size_t size, int align, uint32_t *phys_addr);
uint8_t* kmalloc(size_t size);
uint8_t* krealloc(uint8_t* buff, size_t size);
void kfree(uint8_t* buff);

#endif
