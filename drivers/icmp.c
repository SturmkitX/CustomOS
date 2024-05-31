#include "icmp.h"

#include "../libc/endian.h"
#include "../libc/string.h"

static const char* DefaultPayload = "abcdefghijklmnopqrstuvwabcdefghi";

uint16_t calculateICMPEchoChecksum(struct ICMPEchoPacket* icmpHeader) {
    return calculateHeaderChecksum(icmpHeader, ICMP_HEADER_LEN + strlen(DefaultPayload), (uint16_t*)(&icmpHeader->header.type), &icmpHeader->header.checksum);
}

void constructICMPEcho(struct ICMPEchoPacket* icmp, union IPAddress* destip, uint16_t sequence) {
    // also need to add IP header
    constructIPPacket(&icmp->header.ip, 32 + 8, 0x1, destip);   // 40 bytes long ICMP, Protocol 1 (ICMP)

    icmp->header.type = 8;  // 8 = request, 0 = reply
    icmp->header.code = 0;  // should investigate this

    icmp->id = little_to_big_endian_word(1);   // use a fixed value for now
    icmp->seq = little_to_big_endian_word(sequence);
    memory_copy(icmp->payload, DefaultPayload, strlen(DefaultPayload));

    icmp->header.checksum = calculateICMPEchoChecksum(icmp);
}