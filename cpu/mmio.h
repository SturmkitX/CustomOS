#ifndef _MMIO_H
#define _MMIO_H

#include <stdint.h>

uint8_t mmio_byte_in(uintptr_t addr);
uint16_t mmio_word_in(uintptr_t addr);
uint32_t mmio_dword_in(uintptr_t addr);

void mmio_byte_out(uintptr_t addr, uint8_t value);
void mmio_word_out(uintptr_t addr, uint16_t value);
void mmio_dword_out(uintptr_t addr, uint32_t value);

#endif
