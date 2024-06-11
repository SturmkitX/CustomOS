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

    uintptr_t payload;
    uint16_t payloadSize;
};

void constructUDPHeader(struct UDPPacket* udp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength);
uint16_t calculateUDPChecksum(struct UDPPacket* udpHeader);
void sendUDP(struct UDPPacket *udp);

void convertUDPEndianness(struct UDPPacket* udp);
void generateUDPHeaderBytes(struct UDPPacket* udp, uintptr_t buffer);
uint16_t getUDPPacketSize(struct UDPPacket* udp);
uintptr_t parseUDPPacket(uintptr_t buffer, struct UDPPacket* udp);
void handleUDPPacketRecv(uintptr_t buffer, struct IPPacket* ip);

#endif
