#include "dns.h"
#include "udp.h"
#include "../libc/mem.h"
#include "../libc/string.h"
#include "../libc/endian.h"

void constructDNSHeader(struct DNSPacket* dns, uintptr_t payload, uint16_t payloadLength) {
    dns->id = 1;    // use fixed value for now, will update later
    dns->flags = 0; // this needs to be tuned, for now I think everything 0 should suffice
    dns->no_questions = 1;  // we can only ask for 1 address for now
    dns->no_answer_rr = 0;
    dns->no_authority_rr = 0;
    dns->no_additional_rr = 0;

    // create payload before creating UDP header
    uintptr_t dnsEnd = (uintptr_t)(dns + 1);

    // need to insert the labels
    char curname[24];   // limit labels to 24 bytes
    curname[0] = 0;
    char* dnsEntry = (char*) payload;

    uint32_t i;
    uintptr_t dnsEndP = dnsEnd;
    for (i=0; i < payloadLength; i++) {
        if (dnsEntry[i] != '.') {
            append(curname, dnsEntry[i]);
        } else {
            kprintf("Found label: %s\n", curname);
            *(uint8_t*)(dnsEndP) = strlen(curname);
            memory_copy(dnsEndP + 1, curname, strlen(curname));
            dnsEndP += (strlen(curname) + 1);
            curname[0] = '\0';
        }
    }
    kprintf("Found label: %s\n", curname);
    *(uint8_t*)(dnsEndP) = strlen(curname);
    memory_copy(dnsEndP + 1, curname, strlen(curname));
    dnsEndP += (strlen(curname) + 1);
    curname[0] = '\0';
    // memory_copy(dnsEnd, payload, payloadLength);

    // jump over payload zone for now
    uint16_t* queryParams = (uint16_t*)dnsEndP;
    *queryParams = 1;           // Query type (A)
    *(queryParams + 1) = 1;     // Class IN

    // Config DNS server
    union IPAddress dnssrv;
    dnssrv.integerForm = 3232238081;

    constructUDPHeader(&dns->udp, &dnssrv, 50001, 53, &dns->id, sizeof(struct DNSPacket) - sizeof(struct UDPPacket) + payloadLength + 4);
}

void sendDNS(struct DNSPacket* dns, char* name, uint16_t payloadLength) {
    // memory_copy((uintptr_t)(dns + 1), name, payloadLength);

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