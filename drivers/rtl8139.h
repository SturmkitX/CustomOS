#ifndef _RTL8139_H
#define _RTL8139_H

#include <stdint.h>

uint32_t identifyRTL8139();
uint8_t* getMACAddress(uint32_t baseAddress);

#endif