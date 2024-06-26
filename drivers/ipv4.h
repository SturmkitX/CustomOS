#ifndef _IPV4_H
#define _IPV4_H

#include <stdint.h>
#include "ethernet.h"

#define IP_HEADER_LEN 20

struct IPPacket {
    struct EthernetFrame eth;
    uint8_t version_header;
    uint8_t tos;
    uint16_t total_length;  // Header + Payload
    uint16_t identification;
    uint16_t flags_fragment_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    union IPAddress srcip;
    union IPAddress dstip;

    // options, variable length
};

uint16_t calculateIPChecksum(struct IPPacket* ipHeader);
uint16_t calculateHeaderChecksum(struct IPPacket* ipHeader);

uint32_t calculateHeaderChecksumPhase1(struct IPPacket* ipHeader);
uint16_t wrapHeaderChecksum(uint32_t checksum);
void constructIPPacket(struct IPPacket* ip, uint16_t payload_len, uint8_t protocol, union IPAddress* dstip);

void convertIPPacketEndianness(struct IPPacket* ip);
void generateIPHeaderBytes(struct IPPacket* ip, uintptr_t buffer);
uintptr_t parseIPHeader(uintptr_t buffer, struct IPPacket* ip);
void handleIPPacketRecv(uintptr_t buffer, struct EthernetFrame* eth);

#endif
