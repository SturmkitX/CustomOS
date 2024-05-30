#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <stdint.h>

uint16_t low_to_big_endian_word(uint16_t n);
uint32_t low_to_big_endian_dword(uint32_t n);

#endif