#include "ethernet.h"

#include "../libc/mem.h"
#include "../libc/endian.h"

uint32_t _internal_ip_addr = 167772687;         // 10.0.2.15
uint32_t _internal_subnet_mask = 0xFFFFFF00;    // 255.255.255.0 ??
uint32_t _internal_gateway = 167772674;         // 10.0.2.2

union IPAddress* getIPAddress() {
    return (union IPAddress*)(&_internal_ip_addr);
}

uint32_t getSubnetMask() {
    return _internal_subnet_mask;
}

union IPAddress* getGateway() {
    return (union IPAddress*)(&_internal_gateway);
}

void constructEthernetBroadcast(struct EthernetFrame* eth, uint16_t ethtype) {
    uint32_t i;

    memory_copy(eth->srchw, getMACAddress(), 6);

    for (i=0; i < 6; i++)
        eth->dsthw[i] = 0xFF;
    eth->ethtype = little_to_big_endian_word(ethtype);
}

void constructEthernetFrame(struct EthernetFrame* eth, uint8_t* destMAC, uint16_t ethtype) {
    uint32_t i;

    memory_copy(eth->srchw, getMACAddress(), 6);
    memory_copy(eth->dsthw, destMAC, 6);
    eth->ethtype = little_to_big_endian_word(ethtype);
}
