#ifndef _SERIAL_H
#define _SERIAL_H

#include <stdint.h>

#define COM_PORT 0x3f8          // COM1
 
int init_serial();
int serial_received();
char read_serial();
int is_transmit_empty();
void write_serial(uint8_t a);
void write_string_serial(char* a);

#endif