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
    uint8_t srcip[4];
    uint8_t dstip[4];

    // options, variable length
};

uint16_t calculateIPChecksum(struct IPPacket* ipHeader);
uint16_t calculateHeaderChecksum(struct IPPacket* ipHeader, uint16_t len, uint16_t* buff, uintptr_t checksumAddr);
void constructIPPacket(struct IPPacket* ip, uint16_t payload_len, uint8_t protocol, union IPAddress* dstip);

#endif
