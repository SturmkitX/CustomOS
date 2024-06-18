#ifndef _ICMP_H
#define _ICMP_H

#include <stdint.h>
#include "ipv4.h"

#define ICMP_HEADER_LEN 8

struct ICMPPacket {
    struct IPPacket ip;
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
};

struct ICMPEchoPacket {
    struct ICMPPacket header;
    uint16_t id;
    uint16_t seq;
    char payload[32];   // take a 32 bytes long payload (just like I saw in Windows :)))
    uint16_t payloadSize;
};

struct ICMPDestinationUnreachablePacket {
    struct ICMPPacket header;
    uint32_t empty;
    // uint16_t next_mtu;
    struct IPPacket original_packet;
};

void constructICMPEcho(struct ICMPEchoPacket* icmp, union IPAddress* destip, uint16_t sequence);
uint16_t calculateICMPEchoChecksum(struct ICMPEchoPacket* icmpHeader);

void convertICMPEchoEndianness(struct ICMPEchoPacket* icmp);

void sendICMPEcho(struct ICMPEchoPacket* icmp);
void generateICMPEchoHeaderBytes(struct ICMPEchoPacket* icmp, uintptr_t buffer);
uintptr_t parseICMPPacket(uintptr_t buffer, struct ICMPPacket* icmp);
uint16_t getICMPEchoPacketSize(struct ICMPEchoPacket* icmp);
void handleICMPPacketRecv(uintptr_t buffer, struct IPPacket* ip);

#endif
