#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void memory_copy(void *dest, void *source, int nbytes);
void memory_set(void *dest, uint8_t val, uint32_t len);

/* At this stage there is no 'free' implemented. */
void* kmalloc2(size_t size, int align, uint32_t *phys_addr);
void* kmalloc(size_t size);
void* krealloc(void* buff, size_t size);
void kfree(void* buff);

#endif
