#include "icmp.h"

#include "../libc/endian.h"
#include "../libc/string.h"
#include "../drivers/screen.h"

#include "udp.h"

static const char* DefaultPayload = "abcdefghijklmnopqrstuvwabcdefghi";

uint16_t calculateICMPEchoChecksum(struct ICMPEchoPacket* icmpHeader) {
    // return calculateHeaderChecksum(icmpHeader, ICMP_HEADER_LEN + strlen(DefaultPayload), (uint16_t*)(&icmpHeader->header.type), &icmpHeader->header.checksum);
    uint32_t sum = 0;

    sum += ((icmpHeader->header.type << 8) | icmpHeader->header.code);
    sum += icmpHeader->id;
    sum += icmpHeader->seq;

    // For now, the default 32 bytes long payload is used for ICMP
    uint32_t i;
    uint16_t* payloadBuff = (uint16_t*) DefaultPayload;
    for (i=0; i < strlen(DefaultPayload) / 2; i++) {
        uint16_t pula = little_to_big_endian_word(payloadBuff[i]);
        // kprintf("Pula : %x\n", pula);
        sum += pula;
    }

    return wrapHeaderChecksum(sum);
}

void constructICMPEcho(struct ICMPEchoPacket* icmp, union IPAddress* destip, uint16_t sequence) {
    // also need to add IP header
    constructIPPacket(&icmp->header.ip, 32 + 8, 0x1, destip);   // 40 bytes long ICMP, Protocol 1 (ICMP)

    icmp->header.type = 8;  // 8 = request, 0 = reply
    icmp->header.code = 0;  // should investigate this

    icmp->id = 1;   // use a fixed value for now
    icmp->seq = sequence;
    memory_copy(icmp->payload, DefaultPayload, strlen(DefaultPayload));
    icmp->payloadSize = strlen(DefaultPayload);

    icmp->header.checksum = calculateICMPEchoChecksum(icmp);
}

void convertICMPEchoEndianness(struct ICMPEchoPacket* icmp) {
    icmp->id = little_to_big_endian_word(icmp->id);
    icmp->seq = little_to_big_endian_word(icmp->seq);

    icmp->header.checksum = little_to_big_endian_word(icmp->header.checksum);

    convertIPPacketEndianness(&icmp->header.ip);
}

void sendICMPEcho(struct ICMPEchoPacket* icmp) {
    uint8_t icmpBytes[128];     // for now, set it to mtu size
    generateICMPEchoHeaderBytes(icmp, icmpBytes);
    uint16_t icmpSize = getICMPEchoPacketSize(icmp);

    kprintf("ICMP Size is: %u\n", icmpSize);
    memory_copy((uintptr_t)(icmpBytes + icmpSize - icmp->payloadSize), icmp->payload, icmp->payloadSize);

    transmit_packet(icmpBytes, icmpSize);
}

void generateICMPEchoHeaderBytes(struct ICMPEchoPacket* icmp, uintptr_t buffer) {
    generateIPHeaderBytes(&icmp->header.ip, buffer);

    // May produce a bug
    uintptr_t bufferICMP = (uintptr_t)(buffer + ETHERNET_HEADER_LEN + IP_HEADER_LEN);
    *(uint8_t*)(bufferICMP) = icmp->header.type;
    *(uint8_t*)(bufferICMP + 1) = icmp->header.code;
    *(uint16_t*)(bufferICMP + 2) = little_to_big_endian_word(icmp->header.checksum);

    *(uint16_t*)(bufferICMP + 4) = little_to_big_endian_word(icmp->id);
    *(uint16_t*)(bufferICMP + 6) = little_to_big_endian_word(icmp->seq);
    memory_copy(bufferICMP + 8, icmp->payload, icmp->payloadSize);
}

uintptr_t parseICMPPacket(uintptr_t buffer, struct ICMPPacket* icmp) {
    icmp->type = *(uint8_t*)(buffer);
    icmp->code = *(uint8_t*)(buffer + 1);
    icmp->checksum = big_to_little_endian_word(*(uint16_t*)(buffer + 2));

    return (buffer + 4);
}

uint16_t getICMPEchoPacketSize(struct ICMPEchoPacket* icmp) {
    return (icmp->header.ip.total_length + ETHERNET_HEADER_LEN);
}

void handleICMPPacketRecv(uintptr_t buffer, struct IPPacket* ip) {
    struct ICMPPacket icmp;
    memory_copy(&icmp.ip, ip, sizeof(struct IPPacket));
    uintptr_t remainingBuff = parseICMPPacket(buffer, &icmp);
    switch(icmp.type) {
        case 0:     // Echo Reply
            kprintf("Got ICMP Reply from IP %u.%u.%u.%u\n", icmp.ip.srcip.bytes[3], icmp.ip.srcip.bytes[2], icmp.ip.srcip.bytes[1], icmp.ip.srcip.bytes[0]);
            break;
        case 3:     // Destination unreachable
            kprintf("Got ICMP Destination Unreachable from IP %u.%u.%u.%u\n", icmp.ip.srcip.bytes[3], icmp.ip.srcip.bytes[2], icmp.ip.srcip.bytes[1], icmp.ip.srcip.bytes[0]);
            struct IPPacket originalIP;
            remainingBuff = parseIPHeader(remainingBuff + 4, &originalIP);      // 4 bytes are zeroed

            kprintf("Parsed IP Header. Protocol == %u\n", originalIP.protocol);
            if (originalIP.protocol == 17) {
                // Got UDP packet
                struct UDPPacket originalUDP;
                memory_copy(&originalUDP.ip, &originalIP, sizeof(struct IPPacket));
                remainingBuff = parseUDPPacket(remainingBuff, &originalUDP);

                kprintf("Parsed Dest UNREACH for UDP Packet with src port: %u\n", originalUDP.srcport);
                originalUDP.unreachable = 1;

                addUDPPacket(originalUDP.srcport, &originalUDP);
            }
            break;

        default:
            kprint("ICMP Packet type not recognized yet\n");
            
    }

    // icmp->id = big_to_little_endian_word(*(uint16_t*)(buffer + 4));
    // icmp->seq = big_to_little_endian_word(*(uint16_t*)(buffer + 6));

    // uint16_t payloadSize = icmp->header.ip.total_length - IP_HEADER_LEN - ICMP_HEADER_LEN;

    // memory_copy(icmp->payload, remainingBuff, payloadSize);
    // icmp->payloadSize = payloadSize;
}
