#include "udp.h"
#include "../libc/endian.h"
#include "ipv4.h"
#include "../libc/mem.h"

struct UDPEntry* _udp_entries[(1 << 16)];

void constructUDPHeader(struct UDPPacket* udp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength) {
    // also need to add IP header
    constructIPPacket(&udp->ip, UDP_HEADER_LEN + payloadLength, 17, destip);   // variable length UDP, Protocol 17 (UDP, 0x11)

    udp->srcport = srcport;
    udp->dstport = dstport;

    udp->total_length = UDP_HEADER_LEN + payloadLength;
    udp->payload = payload;
    udp->payloadSize = payloadLength;

    udp->unreachable = 0;

    udp->checksum = calculateUDPChecksum(udp);
}

uint16_t calculateUDPChecksum(struct UDPPacket* udpHeader) {
    // 1. Add up pseudo header (Src IP + Dst IP + IP protocol + UDP Length)
    uint32_t sum = 0;
    sum += (udpHeader->ip.srcip.integerForm >> 16);
    sum += (udpHeader->ip.srcip.integerForm & 0xFFFF);


    sum += (udpHeader->ip.dstip.integerForm >> 16);
    sum += (udpHeader->ip.dstip.integerForm & 0xFFFF);

    sum += udpHeader->total_length;
    sum += udpHeader->ip.protocol;

    // 2. Add up UDP header
    sum += udpHeader->srcport;
    sum += udpHeader->dstport;
    sum += udpHeader->total_length;

    // 3. Add up payload
    uint32_t i;
    for (i=0; i < udpHeader->payloadSize - 1; i += 2) {
        uint16_t num = little_to_big_endian_word(*(uint16_t*)(udpHeader->payload + i));
        sum += num;
    }

    if (i == udpHeader->payloadSize - 1) {
        sum += (*(uint8_t*)(udpHeader->payload + udpHeader->payloadSize - 1)) << 8;
    }

    // sum += calculateIPHeaderChecksumPhase1(&udpHeader->ip);

    return wrapHeaderChecksum(sum);
}

void sendUDP(struct UDPPacket *udp) {
    uint8_t udpBytes[1512];     // for now, set it to mtu size
    generateUDPHeaderBytes(udp, udpBytes);
    uint16_t udpSize = getUDPPacketSize(udp);

    kprintf("UDP Size is: %u\n", udpSize);
    memory_copy((uintptr_t)(udpBytes + udpSize - udp->payloadSize), udp->payload, udp->payloadSize);

    transmit_packet(udpBytes, udpSize);
}

void convertUDPEndianness(struct UDPPacket* udp) {
    udp->srcport = little_to_big_endian_word(udp->srcport);
    udp->dstport = little_to_big_endian_word(udp->dstport);
    udp->total_length = little_to_big_endian_word(udp->total_length);
    udp->checksum = little_to_big_endian_word(udp->checksum);

    convertIPPacketEndianness(&udp->ip);
}

void generateUDPHeaderBytes(struct UDPPacket* udp, uintptr_t buffer) {
    generateIPHeaderBytes(&udp->ip, buffer);

    // May produce a bug
    uintptr_t bufferUDP = (uintptr_t)(buffer + ETHERNET_HEADER_LEN + IP_HEADER_LEN);
    *(uint16_t*)(bufferUDP) = little_to_big_endian_word(udp->srcport);
    *(uint16_t*)(bufferUDP + 2) = little_to_big_endian_word(udp->dstport);
    *(uint16_t*)(bufferUDP + 4) = little_to_big_endian_word(udp->total_length);
    *(uint16_t*)(bufferUDP + 6) = little_to_big_endian_word(udp->checksum);
    memory_copy(bufferUDP + 8, udp->payload, udp->payloadSize);
}

uint16_t getUDPPacketSize(struct UDPPacket* udp) {
    return (udp->ip.total_length + ETHERNET_HEADER_LEN);
}

uintptr_t parseUDPPacket(uintptr_t buffer, struct UDPPacket* udp) {
    udp->srcport = little_to_big_endian_word(*(uint16_t*)(buffer));
    udp->dstport = little_to_big_endian_word(*(uint16_t*)(buffer + 2));
    udp->total_length = little_to_big_endian_word(*(uint16_t*)(buffer + 4));
    udp->checksum = little_to_big_endian_word(*(uint16_t*)(buffer + 6));

    uint16_t payloadSize = udp->ip.total_length - IP_HEADER_LEN - UDP_HEADER_LEN;

    uintptr_t payloadBuff = (uintptr_t)kmalloc(payloadSize);
    memory_copy(payloadBuff, buffer + 8, payloadSize);

    udp->payload = payloadBuff;
    udp->payloadSize = payloadSize;
    udp->unreachable = 0;

    return (buffer + UDP_HEADER_LEN + payloadSize);
}

void handleUDPPacketRecv(uintptr_t buffer, struct IPPacket* ip) {
    struct UDPPacket udp;
    memory_copy(&udp.ip, ip, sizeof(struct IPPacket));
    uintptr_t remainingBuff = parseUDPPacket(buffer, &udp);

    // register UDP packet
    addUDPPacket(udp.dstport, &udp);
}

struct UDPPacket* pollUDP(uint16_t port) {
    if (_udp_entries[port] == NULL)
        return NULL;
    
    struct UDPEntry* entry = _udp_entries[port];
    if (entry->current_ptr == entry->size)
        return NULL;

    return &entry->udp[entry->current_ptr++];
}

void addUDPPacket(uint16_t port, struct UDPPacket* udp) {
    if (_udp_entries[port] == NULL) {
        kprintf("Creating Entries for UDP Port: %u\n", port);
        _udp_entries[port] = (struct UDPEntry*) kmalloc(sizeof(struct UDPEntry));
        _udp_entries[port]->size = 0;
        _udp_entries[port]->current_ptr = 0;
    }
    
    struct UDPEntry* entry = _udp_entries[port];

    // if (entry == NULL) {
    //     kprintf("FATAL ERROR! AddUDPPacket failed to add packet with payload len %u on port %u\n", udp->payloadSize, port);
    //     asm volatile("hlt");
    // }
    // for now, we only accept packets from one connection
    // the packet should be already dropped if its seq does not match what we acknowledge
    
    memory_copy(&entry->udp[entry->size++], udp, sizeof(struct UDPPacket));     // the object is already allocated in memory
}
