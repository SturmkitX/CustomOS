#include "keyboard.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "screen.h"
#include "../libc/string.h"
#include "../libc/function.h"
#include "../kernel/kernel.h"
#include <stdint.h>

#define BACKSPACE 0x0E
#define ENTER 0x1C

static char key_buffer[256];

static volatile uint8_t _sc_buffer[256];
static volatile uint8_t _sc_buffer_left = 0, _sc_buffer_right = 0;

#define SC_MAX 57
const char *sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6", 
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E", 
        "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl", 
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", 
        "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", 
        "/", "RShift", "Keypad *", "LAlt", "Spacebar",
        "Caps Lock", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
        "Num Lock", "Scroll Lock", "Keypad 7", "Keypad 8", "Keypad 9", "Keypad -",
        "Keypad 4", "Keypad 5", "Keypad 6", "Keypad +",
        "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 0", "Numpad .",
        "", "", "", "F11", "F12"};
const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 
        'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G', 
        'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V', 
        'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '};

const char sc_mapped[] = { -1, 27, '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', 127, 9, 'q', 'w', 'e', 'r', 't', 'y', 
        'u', 'i', 'o', 'p', '[', ']', 13,(0x80 + 0x1d), 'a', 's', 'd', 'f', 'g', 
        'h', 'j', 'k', 'l', ';', '\'', '`', (0x80 + 0x36), '\\', 'z', 'x', 'c', 'v', 
        'b', 'n', 'm', ',', '.', '/', (0x80 + 0x36), '*', (0x80 + 0x38), ' ',
        ' ', (0x80 + 0x3b), (0x80 + 0x3c), (0x80 + 0x3d), (0x80 + 0x3e), (0x80 + 0x3f), (0x80 + 0x40), (0x80 + 0x41), (0x80 + 0x42), (0x80 + 0x43), (0x80 + 0x44),
        ' ', ' ', '7', '8', '9', '-',
        '4', '5', '6', '+',
        '1', '2', '3', '0', '.',
        ' ', ' ', ' ', (0x80 + 0x57), (0x80 + 0x58)};

static void keyboard_callback(registers_t *regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    uint8_t scancode = port_byte_in(0x60);

    // let's use the int overflow for now
    _sc_buffer[_sc_buffer_right++] = scancode;
    // kprintf("Added scancode %x, key %s, ASCII = %c\n", scancode, sc_name[scancode], key_to_ascii(scancode, 0));
    
    // if (scancode > SC_MAX) return;
    if (scancode == BACKSPACE) {
        backspace(key_buffer);
        kprint_backspace();
    } else if (scancode == ENTER) {
        kprint("\n");
        user_input(key_buffer); /* kernel-controlled function */
        key_buffer[0] = '\0';
    } else if (scancode <= SC_MAX) {   // print only if key is pressed, not released
        char letter = sc_ascii[(int)scancode];
        /* Remember that kprint only accepts char[] */
        char str[2] = {letter, '\0'};
        append(key_buffer, letter);
        kprint(str);
    }
    UNUSED(regs);
}

void init_keyboard() {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}

uint8_t poll_key(char* key, uint8_t* released, uint8_t* special) {
    if (_sc_buffer_left == _sc_buffer_right)
        return 0;

    *special = 0;
    uint8_t sc = _sc_buffer[_sc_buffer_left++];

    if (sc == 0xe0) {
        *special = 1;
        sc = _sc_buffer[_sc_buffer_left++];
    }

    if (sc < 0x80) {
        *key = sc;
        *released = 0;
    } else {
        *key = sc - 0x80;
        *released = 1;
    }

    return 1;
}

char key_to_ascii(char key, uint8_t special) {
    // kprintf("Is special %x %u\n", key, special);
    if (special) {
        switch(key) {
            case 0x48:
                return 0xad;    // UP Arrow
            case 0x50:
                return 0xaf;    // Down arrow
            case 0x4b:
                return 0xac;    // Left Arrow
            case 0x4d:
                return 0xae;
        }
    }

    return sc_mapped[key];
}
