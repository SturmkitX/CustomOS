#include "tcp.h"

#include <stddef.h>
#include "../libc/mem.h"
#include "../libc/endian.h"

void constructTCPHeader(struct TCPPacket* tcp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength, uint8_t isSyn) {
    uint16_t totalTCPLen = TCP_HEADER_LEN + 4 + payloadLength;
    tcp->srcport = srcport;
    tcp->dstport = dstport;

    tcp->seq = 1;
    tcp->ack = 0;
    tcp->offset_and_flags = (uint16_t)((totalTCPLen << 10) | (isSyn ? TCP_FLAG_SYN : 0));
    tcp->window = 512;  // set Window at 512 bytes
    tcp->urgent_ptr = 0;
    tcp->mss = 1400;    // set Max segment size to 1400
    tcp->msskey = 2;
    tcp->msslength = 4;

    kprintf("TCP LEN: %x %x %x\n", totalTCPLen, (totalTCPLen << 10), tcp->offset_and_flags);

    tcp->checksum = calculateTCPChecksum(tcp, payload, payloadLength);

    // also need to add IP header
    constructIPPacket(&tcp->ip, totalTCPLen, 6, destip);   // variable length TCP, Protocol 6
}

uint16_t calculateTCPChecksum(struct TCPPacket* tcpHeader, uintptr_t payload, uint16_t payloadLength) {
    union IPAddress* srcip = (union IPAddress*)tcpHeader->ip.srcip;
    union IPAddress* dstip = (union IPAddress*)tcpHeader->ip.dstip;

    // 1. Add up pseudo header (Src IP + Dst IP + IP protocol + TCP Length)
    uint32_t sum = 0;
    sum += (srcip->integerForm >> 16);
    sum += (srcip->integerForm & 0xFFFF);


    sum += (dstip->integerForm >> 16);
    sum += (dstip->integerForm & 0xFFFF);

    sum += (TCP_HEADER_LEN + 4 + payloadLength);
    sum += tcpHeader->ip.protocol;

    // 2. Add up TCP header
    sum += tcpHeader->srcport;
    sum += tcpHeader->dstport;
    sum += (tcpHeader->seq >> 16);  // add low and high words separately
    sum += (tcpHeader->seq & 0xFF);
    sum += (tcpHeader->ack >> 16);
    sum += (tcpHeader->ack & 0xFF);
    sum += tcpHeader->offset_and_flags;
    sum += tcpHeader->window;
    sum += tcpHeader->urgent_ptr;

    // no need to add MSS (I believe)

    // 3. Add up payload, if present
    if (payload != NULL) {
        uint32_t i;
        for (i=0; i < payloadLength - 1; i += 2) {
            uint16_t num = little_to_big_endian_word(*(uint16_t*)(payload + i));
            sum += num;
        }

        if (i == payloadLength - 1) {
            sum += (*(uint8_t*)(payload + payloadLength - 1)) << 8;
        }
    }
    // sum += calculateIPHeaderChecksumPhase1(&tcpHeader->ip);

    return wrapHeaderChecksum(sum);
}

void sendTCP(struct TCPPacket *tcp, uintptr_t buff, uint16_t buffLen) {
    memory_copy((uintptr_t)(tcp + 1), buff, buffLen);

    convertTCPEndianness(tcp);

    transmit_packet(tcp, sizeof(struct TCPPacket) + 4 + buffLen);
}

void convertTCPEndianness(struct TCPPacket* tcp) {
    tcp->srcport = little_to_big_endian_word(tcp->srcport);
    tcp->dstport = little_to_big_endian_word(tcp->dstport);
    tcp->seq = little_to_big_endian_dword(tcp->seq);
    tcp->ack = little_to_big_endian_dword(tcp->ack);
    tcp->checksum = little_to_big_endian_word(tcp->checksum);
    tcp->offset_and_flags = little_to_big_endian_word(tcp->offset_and_flags);
    tcp->window = little_to_big_endian_word(tcp->window);
    tcp->urgent_ptr = little_to_big_endian_word(tcp->urgent_ptr);
    tcp->mss = little_to_big_endian_word(tcp->mss);

    convertIPPacketEndianness(&tcp->ip);
}
