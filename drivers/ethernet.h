#ifndef _ETHERNET_H
#define _ETHERNET_H

#include <stdint.h>

// Note that Ethernet and all subsequent protocols use Big Endian

union IPAddress {
    uint8_t bytes[4];
    uint32_t integerForm;
};

struct EthernetFrame
{
    uint8_t dsthw[6];
    uint8_t srchw[6];
    uint16_t ethtype;
};

uint8_t* getDummyIP();
void constructEthernetBroadcast(struct EthernetFrame* eth);
uint8_t* getMACAddress();
void transmit_packet(void* buffer, uint16_t bufLenth);

#endif
