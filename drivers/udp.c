#include "udp.h"
#include "../libc/endian.h"

void constructUDPHeader(struct UDPPacket* udp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uint16_t payloadLength) {
    // also need to add IP header
    constructIPPacket(&udp->ip, UDP_HEADER_LEN + payloadLength, 17, destip);   // variable length UDP, Protocol 17 (UDP, 0x11)

    udp->srcport = little_to_big_endian_word(srcport);
    udp->dstport = little_to_big_endian_word(dstport);

    udp->total_length = little_to_big_endian_word(UDP_HEADER_LEN + payloadLength);

    udp->checksum = calculateUDPChecksum(udp);
}

uint16_t calculateUDPChecksum(struct UDPPacket* udpHeader) {
    return calculateHeaderChecksum(udpHeader, udpHeader->total_length, (uint16_t*)(&udpHeader->srcport), &udpHeader->checksum);
}

void sendUDP(struct UDPPacket *udp, uintptr_t buff, uint16_t buffLen) {
    memory_copy((uintptr_t)(udp + 1), buff, buffLen);

    transmit_packet(udp, sizeof(struct UDPPacket) + buffLen);
}
