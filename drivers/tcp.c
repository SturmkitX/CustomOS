#include "tcp.h"

#include <stddef.h>
#include <stdint.h>
#include "../libc/mem.h"
#include "../libc/endian.h"

struct TCPEntry* _tcp_entries[(1 << 16)];
static uint32_t _current_seq = 1001;
static uint32_t _current_ack = 0;

void constructTCPHeader(struct TCPPacket* tcp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength, uint8_t isSyn, uint8_t isAck) {
    uint16_t totalTCPLen = TCP_HEADER_LEN + payloadLength - 8 * !isSyn;
    tcp->srcport = srcport;
    tcp->dstport = dstport;

    tcp->seq = _current_seq++;
    tcp->ack = _current_ack++;
    tcp->offset_and_flags = (uint16_t)((totalTCPLen << 10) | (isAck ? TCP_FLAG_ACK : 0) | (isSyn ? TCP_FLAG_SYN : 0));
    tcp->window = 512;  // set Window at 512 bytes
    tcp->urgent_ptr = 0;
    tcp->mss = 1400;    // set Max segment size to 1400

    kprintf("TCP LEN: %x %x %x\n", totalTCPLen, (totalTCPLen << 10), tcp->offset_and_flags);


    // also need to add IP header
    constructIPPacket(&tcp->ip, totalTCPLen, 6, destip);   // variable length TCP, Protocol 6

    tcp->checksum = calculateTCPChecksum(tcp, payload, payloadLength, isSyn);
}

uint16_t calculateTCPChecksum(struct TCPPacket* tcpHeader, uintptr_t payload, uint16_t payloadLength, uint8_t isSyn) {
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
    if (isSyn == 1) {
        sum += ((MSS_KIND << 8) | MSS_LENGTH);
        sum += tcpHeader->mss;
        sum += ((NOP_KIND << 8) | NOP_KIND);
        sum += ((SACK_KIND << 8) | SACK_LENGTH);
    }
    

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

uintptr_t parseTCPPacket(uintptr_t buffer, struct TCPPacket* tcp) {
    tcp->srcport = big_to_little_endian_word(*(uint16_t*)(buffer));
    tcp->dstport = big_to_little_endian_word(*(uint16_t*)(buffer + 2));
    tcp->seq = big_to_little_endian_dword(*(uint32_t*)(buffer + 4));
    tcp->ack = big_to_little_endian_dword(*(uint32_t*)(buffer + 8));
    tcp->offset_and_flags = big_to_little_endian_word(*(uint16_t*)(buffer + 12));
    tcp->window = big_to_little_endian_word(*(uint16_t*)(buffer + 14));
    tcp->checksum = big_to_little_endian_word(*(uint16_t*)(buffer + 16));
    tcp->urgent_ptr = big_to_little_endian_word(*(uint16_t*)(buffer + 18));

    tcp->mss = big_to_little_endian_word(*(uint16_t*)(buffer + 22));   // Max segment size

    uint16_t payloadSize = tcp->ip.total_length - IP_HEADER_LEN - TCP_HEADER_LEN;
    uintptr_t payloadBuff = (uintptr_t)kmalloc(payloadSize);
    memory_copy(payloadBuff, buffer + 28, payloadSize);

    tcp->payload = payloadBuff;

    return (buffer + TCP_HEADER_LEN + payloadSize);
}

struct TCPPacket* pollTCP(uint16_t port) {
    if (_tcp_entries[port] == NULL)
        return NULL;
    
    struct TCPEntry* entry = _tcp_entries[port];
    if (entry->current_ptr == entry->size)
        return NULL;

    return &entry->tcp[entry->current_ptr++];
}

void addTCPPacket(uint16_t port, struct TCPPacket* tcp) {
    if (_tcp_entries[port] == NULL) {
        kprintf("Creating Entries for TCP Port: %u\n", port);
        _tcp_entries[port] = (struct TCPEntry*) kmalloc(sizeof(struct TCPEntry));
        _tcp_entries[port]->size = 0;
        _tcp_entries[port]->current_ptr = 0;
    }

    struct TCPEntry* entry = _tcp_entries[port];
    memory_copy(&entry->tcp[entry->size++], tcp, sizeof(struct TCPPacket));     // the object is already allocated in memory
}
