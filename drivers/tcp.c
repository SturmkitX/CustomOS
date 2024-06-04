#include "tcp.h"

#include <stddef.h>
#include <stdint.h>
#include "../libc/mem.h"
#include "../libc/endian.h"

void constructTCPHeader(struct TCPPacket* tcp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength, uint8_t isSyn) {
    uint16_t totalTCPLen = TCP_HEADER_LEN + payloadLength;
    tcp->srcport = srcport;
    tcp->dstport = dstport;

    tcp->seq = 1001;
    tcp->ack = 0;
    tcp->offset_and_flags = (uint16_t)((totalTCPLen << 10) | (isSyn ? TCP_FLAG_SYN : 0));
    tcp->window = 512;  // set Window at 512 bytes
    tcp->urgent_ptr = 0;
    tcp->mss = 1400;    // set Max segment size to 1400

    kprintf("TCP LEN: %x %x %x\n", totalTCPLen, (totalTCPLen << 10), tcp->offset_and_flags);


    // also need to add IP header
    constructIPPacket(&tcp->ip, totalTCPLen, 6, destip);   // variable length TCP, Protocol 6

    tcp->checksum = calculateTCPChecksum(tcp, payload, payloadLength);
}

uint16_t calculateTCPChecksum(struct TCPPacket* tcpHeader, uintptr_t payload, uint16_t payloadLength) {
    // 1. Add up pseudo header (Src IP + Dst IP + IP protocol + TCP Length)
    uint32_t sum = 0;
    sum += (tcpHeader->ip.srcip.integerForm >> 16);
    sum += (tcpHeader->ip.srcip.integerForm & 0xFFFF);


    sum += (tcpHeader->ip.dstip.integerForm >> 16);
    sum += (tcpHeader->ip.dstip.integerForm & 0xFFFF);

    sum += tcpHeader->ip.total_length - IP_HEADER_LEN;
    sum += tcpHeader->ip.protocol;

    // 2. Add up TCP header
    sum += tcpHeader->srcport;
    sum += tcpHeader->dstport;
    sum += (tcpHeader->seq >> 16);  // add low and high words separately
    sum += (tcpHeader->seq & 0xFFFF);
    sum += (tcpHeader->ack >> 16);
    sum += (tcpHeader->ack & 0xFFFF);
    sum += tcpHeader->offset_and_flags;
    sum += tcpHeader->window;
    sum += tcpHeader->urgent_ptr;

    // no need to add MSS (I believe)
    sum += ((MSS_KIND << 8) | MSS_LENGTH);
    sum += tcpHeader->mss;
    sum += ((NOP_KIND << 8) | NOP_KIND);
    sum += ((SACK_KIND << 8) | SACK_LENGTH);

    // 3. Add up payload, if present
    if (payload != NULL && payloadLength > 0) {
        uint32_t i;
        for (i=0; i < payloadLength - 1; i += 2) {
            uint16_t num = little_to_big_endian_word(*(uint16_t*)(payload + i));
            sum += num;
        }

        if (i == payloadLength - 1) {
            sum += (*(uint8_t*)(payload + i) << 8);
        }
    }
    // sum += calculateIPHeaderChecksumPhase1(&tcpHeader->ip);

    return wrapHeaderChecksum(sum);
}

void sendTCP(struct TCPPacket *tcp, uintptr_t buff, uint16_t buffLen) {
    // convertTCPEndianness(tcp);

    // kprintf("Ethernet Frame len: %u\n", sizeof(struct EthernetFrame));
    // kprintf("IP Packet len: %u\n", sizeof(struct IPPacket));
    // kprintf("TCP Packet len: %u\n", sizeof(struct TCPPacket));
    uint8_t tcpBytes[1512];     // for now, set it to mtu size
    generateTCPHeaderBytes(tcp, tcpBytes);
    uint16_t tcpSize = getTCPPacketSize(tcp);

    kprintf("TCP Size is: %u\n", tcpSize);
    memory_copy((uintptr_t)(tcpBytes + tcpSize - buffLen), buff, buffLen);

    transmit_packet(tcpBytes, tcpSize);
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

void generateTCPHeaderBytes(struct TCPPacket* tcp, uintptr_t buffer) {
    generateIPHeaderBytes(&tcp->ip, buffer);

    uintptr_t bufferTCP = (uintptr_t)(buffer + ETHERNET_HEADER_LEN + IP_HEADER_LEN);
    *(uint16_t*)(bufferTCP) = little_to_big_endian_word(tcp->srcport);
    *(uint16_t*)(bufferTCP + 2) = little_to_big_endian_word(tcp->dstport);
    *(uint32_t*)(bufferTCP + 4) = little_to_big_endian_dword(tcp->seq);
    *(uint32_t*)(bufferTCP + 8) = little_to_big_endian_dword(tcp->ack);
    *(uint16_t*)(bufferTCP + 12) = little_to_big_endian_word(tcp->offset_and_flags);
    *(uint16_t*)(bufferTCP + 14) = little_to_big_endian_word(tcp->window);
    *(uint16_t*)(bufferTCP + 16) = little_to_big_endian_word(tcp->checksum);
    *(uint16_t*)(bufferTCP + 18) = little_to_big_endian_word(tcp->urgent_ptr);

    *(uint8_t*)(bufferTCP + 20) = MSS_KIND;
    *(uint8_t*)(bufferTCP + 21) = MSS_LENGTH;
    *(uint16_t*)(bufferTCP + 22) = little_to_big_endian_word(tcp->mss);   // Max segment size

    *(uint8_t*)(bufferTCP + 24) = NOP_KIND;
    *(uint8_t*)(bufferTCP + 25) = NOP_KIND;

    *(uint8_t*)(bufferTCP + 26) = SACK_KIND;
    *(uint8_t*)(bufferTCP + 27) = SACK_LENGTH;
}

uint16_t getTCPPacketSize(struct TCPPacket* tcp) {
    return (tcp->ip.total_length + ETHERNET_HEADER_LEN);
}
