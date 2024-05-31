#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>
#include "ethernet.h"

// Note that Ethernet and all subsequent protocols use Big Endian


struct ARP
{
    struct EthernetFrame eth;
    uint16_t htype; // Hardware type
    uint16_t ptype; // Protocol type
    uint8_t  hlen; // Hardware address length (Ethernet = 6)
    uint8_t  plen; // Protocol address length (IPv4 = 4)
    uint16_t opcode; // ARP Operation Code
    uint8_t  srchw[6]; // Source hardware address - hlen bytes (see above)
    uint8_t  srcpr[4]; // Source protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
    uint8_t  dsthw[6]; // Destination hardware address - hlen bytes (see above)
    uint8_t  dstpr[4]; // Destination protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
    uint8_t  padding[18];   // Padding to get minimum size for Ethernet packets (64 bytes)
};

struct ARPEntry {
    union IPAddress ip;
    uint8_t mac[6];
};

void constructARP(struct ARP* arp, union IPAddress* dstaddr);
void sendARP(struct ARP* arp, union IPAddress* ip);
uint8_t* getARPEntry(union IPAddress* addr);
void associateMACAddress(uint8_t* mac);

#endif