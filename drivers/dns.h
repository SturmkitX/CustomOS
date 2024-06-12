#ifndef _DNS_H
#define _DNS_H

#include <stdint.h>
#include "udp.h"

#define DNS_OPCODE_NORMAL 0
#define DNS_OPCODE_IQUERY 1
#define DNS_OPCODE_SVSTATUS 2

#define DNS_HEADER_LEN  12

struct DNSLabel {
    char name[64];
    uint8_t length;
};

struct DNSPacket {
    struct UDPPacket udp;
    uint16_t id;    // can be used for matching request + response
    uint16_t flags;
    uint16_t no_questions;  // number of questions
    uint16_t no_answer_rr;  // set by server
    uint16_t no_authority_rr;   // set by server
    uint16_t no_additional_rr;  // set by server

    struct DNSLabel* labels;
    uint32_t hashcode;
    uint8_t length;
    uint8_t no_labels;
    uint8_t is_response;
    uint16_t query_type;
    uint16_t query_class;
};

struct DNSRecord {
    uint32_t hashcode;
    char name[256];     // Max name is 253 chars
    union IPAddress ip;
};

void constructDNSHeader(struct DNSPacket* dns, uintptr_t payload, uint16_t payloadLength);
void sendDNS(struct DNSPacket* dns, char* name, uint16_t payloadLength);

void convertDNSEndianness(struct DNSPacket* dns, uint16_t payloadLength);
void generateDNSHeaderBytes(struct DNSPacket* dns, uintptr_t buffer);
uint16_t getDNSPacketSize(struct DNSPacket* dns);
uintptr_t parseDNSPacket(uintptr_t buffer, struct DNSPacket* dns);
struct DNSRecord* pollDNS(char* dnsName);
void registerDNSRecord(struct DNSPacket* dns);

#endif
