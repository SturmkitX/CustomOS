#ifndef _VGA_H
#define _VGA_H

#include <stdint.h>

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
#define PIXEL_WIDTH     3
#define STRETCH_FACTOR  1

void putpixel(int pos_x, int pos_y, uint32_t VGA_COLOR);
uint8_t init_vga();
void draw_icon(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uintptr_t pixels);
void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

#endif
