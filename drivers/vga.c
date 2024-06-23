#include "vga.h"

#include "../cpu/ports.h"
#include "../cpu/mmio.h"
#include "screen.h"

#define VGA_START_ADDR  0xA0000     // MMIO

uint8_t init_vga() {
    port_byte_in(0x3DA);    // reset 0x3C0
    // Check if 0x3C6 is 0xFF

    port_byte_out(0x3C0, 0x10);
    port_byte_out(0x3C0, 0x41);     // 320x200, 256 colors mode

    port_byte_out(0x3C0, 0x11);
    port_byte_out(0x3C0, 0);     // Overscan

    port_byte_out(0x3C0, 0x12);
    port_byte_out(0x3C0, 0x0F);

    port_byte_out(0x3C0, 0x13);
    port_byte_out(0x3C0, 0);

    port_byte_out(0x3C0, 0x14);
    port_byte_out(0x3C0, 0);

    port_byte_out(0x3C2, 0x63);

    port_byte_out(0x3C4, 0x01);
    port_byte_out(0x3C5, 0x01);

    port_byte_out(0x3C4, 0x03);
    port_byte_out(0x3C5, 0);

    port_byte_out(0x3C4, 0x04);
    port_byte_out(0x3C5, 0x0E);

    port_byte_out(0x3CE, 0x05);
    port_byte_out(0x3CF, 0x40);

    port_byte_out(0x3CE, 0x06);
    port_byte_out(0x3CF, 0x05);

    port_byte_out(0x3D4, 0);
    port_byte_out(0x3D5, 0x5F);

    port_byte_out(0x3D4, 0x1);
    port_byte_out(0x3D5, 0x4F);

    port_byte_out(0x3D4, 0x2);
    port_byte_out(0x3D5, 0x50);

    port_byte_out(0x3D4, 0x3);
    port_byte_out(0x3D5, 0x82);

    port_byte_out(0x3D4, 0x4);
    port_byte_out(0x3D5, 0x54);

    port_byte_out(0x3D4, 0x5);
    port_byte_out(0x3D5, 0x80);

    port_byte_out(0x3D4, 0x6);
    port_byte_out(0x3D5, 0xBF);

    port_byte_out(0x3D4, 0x7);
    port_byte_out(0x3D5, 0x1F);

    port_byte_out(0x3D4, 0x8);
    port_byte_out(0x3D5, 0);

    port_byte_out(0x3D4, 0x9);
    port_byte_out(0x3D5, 0x41);

    port_byte_out(0x3D4, 0x10);
    port_byte_out(0x3D5, 0x9C);

    port_byte_out(0x3D4, 0x11);
    port_byte_out(0x3D5, 0x8E);

    port_byte_out(0x3D4, 0x12);
    port_byte_out(0x3D5, 0x8F);

    port_byte_out(0x3D4, 0x13);
    port_byte_out(0x3D5, 0x28);

    port_byte_out(0x3D4, 0x14);
    port_byte_out(0x3D5, 0x40);

    port_byte_out(0x3D4, 0x15);
    port_byte_out(0x3D5, 0x96);

    port_byte_out(0x3D4, 0x16);
    port_byte_out(0x3D5, 0xB9);

    port_byte_out(0x3D4, 0x17);
    port_byte_out(0x3D5, 0xA3);

    uint8_t stat = port_byte_in(0x3C6);
    kprintf("VGA Test Val = %x\n", stat);

    return 1;
}

/* example for 320x200 VGA */
void putpixel(int pos_x, int pos_y, unsigned char VGA_COLOR)
{
    unsigned char* location = (unsigned char*)VGA_START_ADDR + 320 * pos_y + pos_x;
    // *location = VGA_COLOR;
    mmio_byte_out(location, VGA_COLOR);
}

void draw_icon(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uintptr_t pixels) {
    uint32_t i,j,l;
    uint8_t* pxs = (uint8_t*)pixels;
    for (l = j = 0; l < h; l++) {
        for (i = 0; i < w; i++, j++) {
            putpixel(x + i, y + l, pxs[j]);
        }
    }
}
