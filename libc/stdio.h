#ifndef _STDIO_H
#define _STDIO_H

#include <stdint.h>

const uint8_t stdin = 0;
const uint8_t stdout = 1;
const uint8_t stderr = 2;

typedef uintptr_t FILE;

void fprintf(uint8_t channel, char *fmt, ...);

#endif
