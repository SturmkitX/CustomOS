#include "ipv4.h"
#include "../libc/mem.h"
#include "../libc/endian.h"

static uint16_t CurrentIdentification = 1;

uint16_t calculateHeaderChecksum(struct IPPacket* ipHeader, uint16_t len, uint16_t* buff, uintptr_t checksumAddr) {
    uint16_t i;
    uint32_t sum = 0;

    for(i=0; i < len/2; i++) {
        if (buff + i == checksumAddr)     // skip the checksum itself
            continue;

        sum += buff[i];
    }

    if (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);     // generally, it is considered that one pass should be enough
    }

    return (uint16_t)(~sum);
}

uint16_t calculateIPChecksum(struct IPPacket* ipHeader) {
    return calculateHeaderChecksum(ipHeader, IP_HEADER_LEN, (uint16_t*)(&ipHeader->version_header), &ipHeader->header_checksum);
}

void constructIPPacket(struct IPPacket* ip, uint16_t payload_len, uint8_t protocol, union IPAddress* dstip) {
    ip->version_header = 0x45;      // IPv4, 20 bytes long header (5 dwords)
    ip->tos = 0;                    // Nothing special wanted right now
    ip->total_length = 20 + payload_len;    // For a 20 bytes header
    ip->identification = CurrentIdentification++;
    ip->flags_fragment_offset = 0;  // don't think we need it now
    ip->ttl = 128;  // It's ok for now
    ip->protocol = protocol;

    memory_copy(ip->srcip, getDummyIP(), 4);
    memory_copy(ip->dstip, dstip->bytes, 4);

    ip->header_checksum = calculateIPChecksum(ip);

    // we still have to construct the Ethernet Frame
    // we need to get the ARP entry
    constructEthernetFrame(&ip->eth, NULL);
}
