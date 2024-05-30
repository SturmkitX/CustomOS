#include "endian.h"

uint16_t low_to_big_endian_word(uint16_t n) {
    return (uint16_t)(
        ((n & 0xFF) << 8) |
        (n >> 8)
    );
}


uint32_t low_to_big_endian_dword(uint32_t n) {
    return (
        ((n & 0xFF) << 24) |
        (((n >> 8) & 0xFF) << 16) |
        (((n >> 16) & 0xFF) << 8) |
        (n >> 24)
    );
}
