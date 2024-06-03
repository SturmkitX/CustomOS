#ifndef _DNS_H
#define _DNS_H

#include <stdint.h>
#include "udp.h"

#define DNS_OPCODE_NORMAL 0
#define DNS_OPCODE_IQUERY 1
#define DNS_OPCODE_SVSTATUS 2

// messages may be truncated if size exceeds 512 bytes
struct DNSPacket {
    struct UDPPacket udp;
    uint16_t id;    // can be used for matching request + response
    uint16_t flags;
    uint16_t no_questions;  // number of questions
    uint16_t no_answer_rr;  // set by server
    uint16_t no_authority_rr;   // set by server
    uint16_t no_additional_rr;  // set by server
};

void constructDNSHeader(struct DNSPacket* dns, uintptr_t payload, uint16_t payloadLength);
void sendDNS(struct DNSPacket* dns, char* name, uint16_t payloadLength);

void convertDNSEndianness(struct DNSPacket* dns, uint16_t payloadLength);

#endif
