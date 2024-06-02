#include "icmp.h"

#include "../libc/endian.h"
#include "../libc/string.h"

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

    icmp->header.checksum = calculateICMPEchoChecksum(icmp);
}

void convertICMPEchoEndianness(struct ICMPEchoPacket* icmp) {
    icmp->id = little_to_big_endian_word(icmp->id);
    icmp->seq = little_to_big_endian_word(icmp->seq);

    icmp->header.checksum = little_to_big_endian_word(icmp->header.checksum);

    convertIPPacketEndianness(&icmp->header.ip);
}
