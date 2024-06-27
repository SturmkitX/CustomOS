#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>

void init_keyboard();
uint8_t poll_key(char* key, uint8_t* released, uint8_t* special);
char key_to_ascii(char key, uint8_t special);

#endif
