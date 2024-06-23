#ifndef _VGA_H
#define _VGA_H

#include <stdint.h>

void putpixel(int pos_x, int pos_y, unsigned char VGA_COLOR);
uint8_t init_vga();
void draw_icon(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uintptr_t pixels);

#endif
