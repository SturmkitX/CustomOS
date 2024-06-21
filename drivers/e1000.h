#ifndef _E1000_H
#define _E1000_H

#include <stdint.h>

// Driver for Intel 82545EM-A (Copper, Server): 8086:100F
uint8_t identifyE1000();
uint8_t* E1000GetMACAddress();

#endif
