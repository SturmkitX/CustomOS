#include "icmp.h"

#include "../libc/endian.h"
#include "../libc/string.h"

static const char* DefaultPayload = "abcdefghijklmnopqrstuvwabcdefghi";

uint16_t calculateICMPEchoChecksum(struct ICMPEchoPacket* icmpHeader) {
    // return calculateHeaderChecksum(icmpHeader, ICMP_HEADER_LEN + strlen(DefaultPayload), (uint16_t*)(&icmpHeader->header.type), &icmpHeader->header.checksum);
    uint32_t sum = 0;

    sum += ((ipHeader->version_header << 8) | ipHeader->tos);
    sum += ipHeader->total_length;
    sum += ipHeader->identification;
    sum += ipHeader->flags_fragment_offset;
    sum += ((ipHeader->ttl << 8) | ipHeader->tos);
    
    sum += ((ipHeader->srcip[3] << 8) | ipHeader->srcip[2]);
    sum += ((ipHeader->srcip[1] << 8) | ipHeader->srcip[0]);

    sum += ((ipHeader->dstip[3] << 8) | ipHeader->dstip[2]);
    sum += ((ipHeader->dstip[1] << 8) | ipHeader->dstip[0]);

    return sum;
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
