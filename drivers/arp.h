#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>

// Note that Ethernet and all subsequent protocols use Big Endian

union IPAddress {
    uint8_t bytes[4];
    uint32_t integerForm;
};

struct ARP
{
    uint16_t htype; // Hardware type
    uint16_t ptype; // Protocol type
    uint8_t  hlen; // Hardware address length (Ethernet = 6)
    uint8_t  plen; // Protocol address length (IPv4 = 4)
    uint16_t opcode; // ARP Operation Code
    uint8_t  srchw[6]; // Source hardware address - hlen bytes (see above)
    uint8_t  srcpr[4]; // Source protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
    uint8_t  dsthw[6]; // Destination hardware address - hlen bytes (see above)
    uint8_t  dstpr[4]; // Destination protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
};

struct EthARP
{
    uint8_t dsthw[6];
    uint8_t srchw[6];
    uint16_t ethtype;
    struct ARP arp;
};

struct ARP* constructARP(union IPAddress* addr);
struct EthARP* constructEthARP(union IPAddress* addr);

#endif