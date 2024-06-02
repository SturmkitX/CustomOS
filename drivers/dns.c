#include "dns.h"
#include "udp.h"
#include "../libc/mem.h"
#include "../libc/string.h"
#include "../libc/endian.h"

void constructDNSHeader(struct DNSPacket* dns, uint16_t payloadLength) {
    dns->id = 1;    // use fixed value for now, will update later
    dns->flags = 0; // this needs to be tuned, for now I think everything 0 should suffice
    dns->no_questions = 1;  // we can only ask for 1 address for now
    dns->no_answer_rr = 0;
    dns->no_authority_rr = 0;
    dns->no_additional_rr = 0;

    // create payload before creating UDP header
    uintptr_t dnsEnd = (uintptr_t)(dns + 1);
    // memory_copy(dnsEnd, name, strlen(name));

    // jump over payload zone for now
    uint16_t* queryParams = (uint16_t*)(dnsEnd + payloadLength);
    *queryParams = 1;           // Query type
    *(queryParams + 1) = 1;     // Class IN

    // Config DNS server
    union IPAddress dnssrv;
    dnssrv.integerForm = 3232238081;

    constructUDPHeader(&dns->udp, &dnssrv, 50001, 53, dns, sizeof(struct DNSPacket) + payloadLength + 4);
}

void sendDNS(struct DNSPacket* dns, char* name, uint16_t payloadLength) {
    memory_copy((uintptr_t)(dns + 1), name, payloadLength);

    convertDNSEndianness(dns, payloadLength);

    transmit_packet(dns, sizeof(struct DNSPacket) + payloadLength + 4);
}

void convertDNSEndianness(struct DNSPacket* dns, uint16_t payloadLength) {
    dns->id = little_to_big_endian_word(dns->id);    // use fixed value for now, will update later
    dns->flags = little_to_big_endian_word(dns->flags); // this needs to be tuned, for now I think everything 0 should suffice
    dns->no_questions = little_to_big_endian_word(dns->no_questions);  // we can only ask for 1 address for now
    dns->no_answer_rr = little_to_big_endian_word(dns->no_answer_rr);
    dns->no_authority_rr = little_to_big_endian_word(dns->no_authority_rr);
    dns->no_additional_rr = little_to_big_endian_word(dns->no_additional_rr);

    uintptr_t dnsEnd = (uintptr_t)(dns + 1);
    uint16_t* queryParams = (uint16_t*)(dnsEnd + payloadLength);
    *queryParams = little_to_big_endian_word(*queryParams);           // Query type
    *(queryParams + 1) = little_to_big_endian_word(*(queryParams + 1));     // Class IN

    convertUDPEndianness(&dns->udp);
}