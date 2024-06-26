#include "ports.h"
#include <stddef.h>
#include "type.h"

/**
 * Read a byte from the specified port
 */
uint8_t port_byte_in (uint16_t port) {
    uint8_t result;
    /* Inline assembler syntax
     * !! Notice how the source and destination registers are switched from NASM !!
     *
     * '"=a" (result)'; set '=' the C variable '(result)' to the value of register e'a'x
     * '"d" (port)': map the C variable '(port)' into e'd'x register
     *
     * Inputs and outputs are separated by colons
     */
    asm("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

void port_byte_out (uint16_t port, uint8_t data) {
    /* Notice how here both registers are mapped to C variables and
     * nothing is returned, thus, no equals '=' in the asm syntax 
     * However we see a comma since there are two variables in the input area
     * and none in the 'return' area
     */
    asm volatile("out %%al, %%dx" : : "a" (data), "d" (port));
}

uint16_t port_word_in (uint16_t port) {
    uint16_t result;
    asm("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void port_word_out (uint16_t port, uint16_t data) {
    asm volatile("out %%ax, %%dx" : : "a" (data), "d" (port));
}

// may need to be reimplemented using INSB (check OS-DEV ASM)
uint16_t port_words_in (uint16_t port, void *buffer, uint16_t length)
{
    uint16_t result;
    uint16_t *it;
    uint16_t remain = length;
    for (it=(uint16_t*)buffer; it != NULL && remain > 0; it++, remain--)
    {
        asm("in %%dx, %%ax" : "=a" (result) : "d" (port));
        *it = result;
    }

    // ERROR CHECKING MISSING
    
    return remain;
}

void port_words_out (uint16_t port, void *buffer, uint16_t length)
{
    uint16_t *it;
    uint16_t remain = length;
    for (it=(uint16_t*)buffer; it != NULL && remain > 0; it++, remain--)
    {
        asm volatile("out %%ax, %%dx" : : "a" (*it), "d" (port));
    }

    // ERROR CHECKING MISSING
}

uint32_t port_dword_in (uint16_t port) {
    uint32_t result;
    asm("in %%dx, %%eax" : "=a" (result) : "d" (port));
    return result;
}

void port_dword_out (uint16_t port, uint32_t data) {
    asm volatile("out %%eax, %%dx" : : "a" (data), "d" (port));
}
