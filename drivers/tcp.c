#include "tcp.h"

#include <stddef.h>
#include <stdint.h>
#include "../libc/mem.h"
#include "../libc/endian.h"

#define TCP_TX_BUFFER_MAX_SIZE  (1 << 5)

struct TCPEntry* _tcp_entries[(1 << 16)];
static uint32_t _current_seq = 1001;
static uint32_t _current_ack = 0;

struct TCPPacket _tcp_tx_buffer[TCP_TX_BUFFER_MAX_SIZE];
static uint32_t _tcp_tx_size = 0;
static uint32_t _tcp_tx_pos = 0;

void constructTCPHeader(struct TCPPacket* tcp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength, uint8_t isSyn, uint8_t isAck, uint8_t isPush, uint8_t isFin) {
    uint16_t totalTCPLen = TCP_HEADER_LEN - 8 * !isSyn;
    tcp->srcport = srcport;
    tcp->dstport = dstport;

    tcp->seq = _current_seq;
    tcp->ack = _current_ack;
    tcp->offset_and_flags = (uint16_t)((totalTCPLen << 10) | (isPush ? TCP_FLAG_PUSH : 0) | (isAck ? TCP_FLAG_ACK : 0) | (isSyn ? TCP_FLAG_SYN : 0) | (isFin ? TCP_FLAG_FIN : 0));
    tcp->window = 0xEEEE;  // set Window at 512 bytes
    tcp->urgent_ptr = 0;
    tcp->mss = 1400;    // set Max segment size to 1400

    kprintf("TCP LEN: %x %x %x\n", totalTCPLen, (totalTCPLen << 10), tcp->offset_and_flags);
    tcp->payload = payload;
    tcp->payloadSize = payloadLength;


    // also need to add IP header
    constructIPPacket(&tcp->ip, totalTCPLen + payloadLength, 6, destip);   // variable length TCP, Protocol 6

    tcp->checksum = calculateTCPChecksum(tcp, isSyn);
}

uint16_t calculateTCPChecksum(struct TCPPacket* tcpHeader, uint8_t isSyn) {
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
    if (tcpHeader->payload != NULL && tcpHeader->payloadSize > 0) {
        uint32_t i;
        for (i=0; i < tcpHeader->payloadSize - 1; i += 2) {
            uint16_t num = little_to_big_endian_word(*(uint16_t*)(tcpHeader->payload + i));
            sum += num;
        }

        if (i == tcpHeader->payloadSize - 1) {
            sum += (*(uint8_t*)(tcpHeader->payload + i) << 8);
        }
    }
    // sum += calculateIPHeaderChecksumPhase1(&tcpHeader->ip);

    return wrapHeaderChecksum(sum);
}

void sendTCP(struct TCPPacket *tcp) {
    // functionality moved to TX Buffer Handler
    // here, we will just fill the buffer for the handler to execute
    struct TCPPacket* pack = &_tcp_tx_buffer[_tcp_tx_size];
    memory_copy(pack, tcp, sizeof(struct TCPPacket));

    // also make a copy of the payload
    pack->payload = (uintptr_t) kmalloc(pack->payloadSize);
    memory_copy(pack->payload, tcp->payload, tcp->payloadSize);

    _tcp_tx_size++;
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

    uint8_t isSyn = (tcp->offset_and_flags & TCP_FLAG_SYN) != 0;
    uint8_t headerSize = (tcp->offset_and_flags >> 12) << 2;    // to zero out trailing bits
    uint16_t payloadSize = tcp->ip.total_length - IP_HEADER_LEN - headerSize;
    kprintf("%u %u %u\n", tcp->ip.total_length, IP_HEADER_LEN, headerSize);
    // while(1) {}
    uintptr_t payloadBuff = (uintptr_t)kmalloc(payloadSize);
    memory_copy(payloadBuff, buffer + headerSize, payloadSize);

    tcp->payload = payloadBuff;
    tcp->payloadSize = payloadSize;

    return (buffer + headerSize + payloadSize);
}

uint8_t checkTCPConnection(uint16_t port) {
    if (_tcp_entries[port] == NULL)
        return 0;
    
    struct TCPEntry* entry = _tcp_entries[port];
    return entry->established;
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
    struct TCPEntry* entry = _tcp_entries[port];

    if (entry == NULL) {
        kprintf("FATAL ERROR! AddTCPPacket failed to add packet with payload len %u on port %u\n", tcp->payloadSize, port);
        asm volatile("hlt");
    }

    // for now, we only accept packets from one connection
    // the packet should be already dropped if its seq does not match what we acknowledge
    
    memory_copy(&entry->tcp[(entry->size++) % TCP_TX_BUFFER_MAX_SIZE], tcp, sizeof(struct TCPPacket));     // the object is already allocated in memory
}

void handleTCPPacketRecv(uintptr_t buffer, struct IPPacket* ip) {
    struct TCPPacket tcp;
    memory_copy(&tcp.ip, ip, sizeof(struct IPPacket));
    uintptr_t remainingBuff = parseTCPPacket(buffer, &tcp);
    kprintf("TCP Receive port: %u\n", tcp.dstport);
    if ((tcp.offset_and_flags & TCP_FLAG_ACK)) {
        // means we started this connection (probably?)
        // IP is still not validated at this point
        // check and update values
        if (_current_ack != tcp.seq && _current_ack != 0) {
            // drop the package
            kprintf("Dropped TCP package: SEQ: %u, OUR ACK: %u\n", tcp.seq, _current_ack);
            return;
        }

        _current_seq = tcp.ack;
        if (_current_ack == 0) _current_ack = tcp.seq;
        kprintf("Current ACK: %u %u\n", _current_ack, tcp.payloadSize);
        

        if ((tcp.offset_and_flags & TCP_FLAG_FIN)) {
            // Send a FIN back and free the buffer
            kprint("Received FIN Message\n");
            _current_ack += (tcp.payloadSize == 0 ? 1 : tcp.payloadSize);
            struct TCPPacket respTCP;
            constructTCPHeader(&respTCP, &tcp.ip.srcip, tcp.dstport, tcp.srcport, NULL, 0, 0, 1, 0, 1);
            sendTCP(&respTCP);

            kfree(_tcp_entries[tcp.dstport]);
            _tcp_entries[tcp.dstport] = NULL;

            return;
        }

        if ((tcp.offset_and_flags & TCP_FLAG_SYN)) {
            kprint("Received Handshake response... Sending ACK\n");
            _current_ack += (tcp.payloadSize == 0 ? 1 : tcp.payloadSize);
            struct TCPPacket respTCP;
            constructTCPHeader(&respTCP, &tcp.ip.srcip, tcp.dstport, tcp.srcport, NULL, 0, 0, 1, 0, 0);
            sendTCP(&respTCP);

            // Register this connection as established
            uint16_t port = tcp.dstport;
            if (_tcp_entries[port] == NULL) {
                kprintf("Creating Entries for TCP Port: %u\n", port);
                _tcp_entries[port] = (struct TCPEntry*) kmalloc(sizeof(struct TCPEntry));
                _tcp_entries[port]->size = 0;
                _tcp_entries[port]->current_ptr = 0;
                _tcp_entries[port]->established = 1;    // there may be a short delay before the ACK packet is actually sent
            }

            return;
        }

        if ((tcp.offset_and_flags & TCP_FLAG_PUSH)) {
            // _current_ack += (tcp->payloadSize == 0 ? 1 : tcp->payloadSize);
            _current_ack += tcp.payloadSize;
            // Save packet, payload and acknowledge
            kprint("Received TCP Message... Sending ACK\n");
            if (_tcp_entries[tcp.dstport] == NULL)     // if we don't know of this connection, drop the package
                return;

            struct TCPPacket respTCP;
            constructTCPHeader(&respTCP, &tcp.ip.srcip, tcp.dstport, tcp.srcport, NULL, 0, 0, 1, 0, 0);
            sendTCP(&respTCP);
        }
    }

    addTCPPacket(tcp.dstport, &tcp);
}

void tcpTXBufferHandler() {
    if (_tcp_tx_pos > _tcp_tx_size) {
        kprint("FATAL ERROR! TCP Buffer TX Position larger than Size\n");
        asm volatile("hlt");
    }

    if (_tcp_tx_pos == _tcp_tx_size)
        return;

    // for now, handle a single message
    struct TCPPacket* tcp = &_tcp_tx_buffer[_tcp_tx_pos];
    _tcp_tx_pos = (_tcp_tx_pos + 1) % TCP_TX_BUFFER_MAX_SIZE;

    uint8_t tcpBytes[1512];     // for now, set it to mtu size
    generateTCPHeaderBytes(tcp, tcpBytes);
    uint16_t tcpSize = getTCPPacketSize(tcp);

    kprintf("TCP Size is: %u\n", tcpSize);
    memory_copy((uintptr_t)(tcpBytes + tcpSize - tcp->payloadSize), tcp->payload, tcp->payloadSize);

    transmit_packet(tcpBytes, tcpSize);

    // free the payload memory
    // kfree(tcp->payload);
}

uint32_t getTCPBufferSize(uint16_t port) {
    if (_tcp_entries[port] == NULL)
        return NULL;
    
    struct TCPEntry* entry = _tcp_entries[port];
    return (entry->size - entry->current_ptr);
}