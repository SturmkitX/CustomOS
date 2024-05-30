#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <stdint.h>

uint16_t little_to_big_endian_word(uint16_t n);
uint32_t little_to_big_endian_dword(uint32_t n);

#define big_to_little_endian_word(n) little_to_big_endian_word(n)
#define big_to_little_endian_dword(n) little_to_big_endian_dword(n)

#endif