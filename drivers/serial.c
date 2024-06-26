#include "serial.h"
#include "../cpu/ports.h"

int init_serial() {
    port_byte_out(COM_PORT + 1, 0x00);    // Disable all interrupts
    port_byte_out(COM_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    port_byte_out(COM_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    port_byte_out(COM_PORT + 1, 0x00);    //                  (hi byte)
    port_byte_out(COM_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    port_byte_out(COM_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    port_byte_out(COM_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    port_byte_out(COM_PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
    port_byte_out(COM_PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
    
    // Check if serial is faulty (i.e: not same byte as sent)
    if(port_byte_in(COM_PORT + 0) != 0xAE) {
        return 1;
    }
 
    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    port_byte_out(COM_PORT + 4, 0x0F);
    return 0;
}

int serial_received() {
    return port_byte_in(COM_PORT + 5) & 1;
}
 
char read_serial() {
    while (serial_received() == 0);
 
    return port_byte_in(COM_PORT);
}

int is_transmit_empty() {
    return port_byte_in(COM_PORT + 5) & 0x20;
}
 
void write_serial(uint8_t a) {
    while (is_transmit_empty() == 0);
 
    port_byte_out(COM_PORT,a);
}

void write_string_serial(char* a) {
    char *s = a;
    while (*s != '\0') {
        write_serial(*s);
        s++;
    }
}
