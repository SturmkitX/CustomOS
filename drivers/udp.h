#ifndef _UDP_H
#define _UDP_H

#include <stdint.h>
#include "ipv4.h"

#define UDP_HEADER_LEN 8


struct UDPPacket {
    struct IPPacket ip;
    uint16_t srcport;
    uint16_t dstport;
    uint16_t total_length;
    uint16_t checksum;

    // data is of variable size
};

void constructUDPHeader(struct UDPPacket* udp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uint16_t payloadLength);
uint16_t calculateUDPChecksum(struct UDPPacket* udpHeader);
void sendUDP(struct UDPPacket *udp, uintptr_t buff, uint16_t buffLen);

#endif
