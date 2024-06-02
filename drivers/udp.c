#include "udp.h"
#include "../libc/endian.h"
#include "ipv4.h"
#include "../libc/mem.h"

void constructUDPHeader(struct UDPPacket* udp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength) {
    // also need to add IP header
    constructIPPacket(&udp->ip, UDP_HEADER_LEN + payloadLength, 17, destip);   // variable length UDP, Protocol 17 (UDP, 0x11)

    udp->srcport = srcport;
    udp->dstport = dstport;

    udp->total_length = UDP_HEADER_LEN + payloadLength;

    udp->checksum = calculateUDPChecksum(udp, payload, payloadLength);
    // udp->checksum = 0;  // disable for now
}

uint16_t calculateUDPChecksum(struct UDPPacket* udpHeader, uintptr_t payload, uint16_t payloadLength) {
    union IPAddress* srcip = (union IPAddress*)udpHeader->ip.srcip;
    union IPAddress* dstip = (union IPAddress*)udpHeader->ip.dstip;

    // 1. Add up pseudo header (Src IP + Dst IP + IP protocol + UDP Length)
    uint32_t sum = 0;
    sum += (srcip->integerForm >> 16);
    sum += (srcip->integerForm & 0xFFFF);


    sum += (dstip->integerForm >> 16);
    sum += (dstip->integerForm & 0xFFFF);

    sum += udpHeader->total_length;
    sum += udpHeader->ip.protocol;

    // 2. Add up UDP header
    sum += udpHeader->srcport;
    sum += udpHeader->dstport;
    sum += udpHeader->total_length;

    // 3. Add up payload
    uint32_t i;
    for (i=0; i < payloadLength - 1; i += 2) {
        uint16_t num = little_to_big_endian_word(*(uint16_t*)(payload + i));
        sum += num;
    }

    if (i == payloadLength - 1) {
        sum += (*(uint8_t*)(payload + payloadLength - 1)) << 8;
    }

    // sum += calculateIPHeaderChecksumPhase1(&udpHeader->ip);

    return wrapHeaderChecksum(sum);
}

void sendUDP(struct UDPPacket *udp, uintptr_t buff, uint16_t buffLen) {
    memory_copy((uintptr_t)(udp + 1), buff, buffLen);

    convertUDPEndianness(udp);

    transmit_packet(udp, sizeof(struct UDPPacket) + buffLen);
}

void convertUDPEndianness(struct UDPPacket* udp) {
    udp->srcport = little_to_big_endian_word(udp->srcport);
    udp->dstport = little_to_big_endian_word(udp->dstport);
    udp->total_length = little_to_big_endian_word(udp->total_length);
    udp->checksum = little_to_big_endian_word(udp->checksum);

    convertIPPacketEndianness(&udp->ip);
}
