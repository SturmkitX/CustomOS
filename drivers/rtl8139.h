#ifndef _RTL8139_H
#define _RTL8139_H

#include <stdint.h>

// extern uint32_t RTL8139BaseAddress;

uint32_t identifyRTL8139();
uint8_t initializeRTL8139();

#endif