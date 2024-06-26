#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

unsigned char port_byte_in (uint16_t port);
void port_byte_out (uint16_t port, uint8_t data);
unsigned short port_word_in (uint16_t port);
void port_word_out (uint16_t port, uint16_t data);
unsigned short port_words_in (uint16_t port, void *buffer, uint16_t length);
void port_words_out (uint16_t port, void *buffer, uint16_t length);

uint32_t port_dword_in (uint16_t port);
void port_dword_out (uint16_t port, uint32_t data);

#endif
