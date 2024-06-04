#ifndef _ETHERNET_H
#define _ETHERNET_H

#include <stdint.h>

#define ETHERNET_HEADER_LEN     14

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

union IPAddress* getIPAddress();
uint32_t getSubnetMask();
union IPAddress* getGateway();
void constructEthernetBroadcast(struct EthernetFrame* eth, uint16_t ethtype);
void constructEthernetFrame(struct EthernetFrame* eth, uint8_t* destMAC, uint16_t ethtype);
uint8_t* getMACAddress();
void transmit_packet(void* buffer, uint16_t bufLenth);

void convertEthernetFrameEndianness(struct EthernetFrame* eth);
void generateEthernetFrameBytes(struct EthernetFrame* eth, uintptr_t buffer);
uintptr_t parseEthernetFrame(uintptr_t buff, struct EthernetFrame* eth);

#endif
