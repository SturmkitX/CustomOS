#include "dns.h"
#include "udp.h"
#include "../libc/mem.h"
#include "../libc/string.h"
#include "../libc/endian.h"

uint32_t computeHashCode(uintptr_t payload, uint16_t payloadLength) {
    uint32_t sum = 0, factor = (1 << 4);
    uint32_t i;
    char* buff = (char*) payload;
    for (i=0; i < payloadLength; i++) {
        if (buff[i] == '.') factor <<= 1;
        else                sum += buff[i] * factor;
    }

    return sum;
}

void generateDNSLabels(struct DNSPacket* dns, uintptr_t payload, uint16_t payloadLength) {
    dns->labels = (struct DNSLabel*) kmalloc(sizeof(struct DNSLabel) * 64);     // assume no more than 64 labels
    dns->no_labels = 0;

    char curname[64];   // Label max size = 63 bytes
    curname[0] = 0;
    char* dnsEntry = (char*) payload;

    uint32_t i;
    // uintptr_t dnsEndP = dnsEnd;
    for (i=0; i < payloadLength; i++) {
        if (dnsEntry[i] != '.') {
            append(curname, dnsEntry[i]);
        } else {
            struct DNSLabel* labelEntry = &dns->labels[dns->no_labels];
            labelEntry->length = strlen(curname);
            memory_copy(labelEntry->name, curname, labelEntry->length);
            curname[0] = '\0';
            dns->no_labels++;
        }
    }
    struct DNSLabel* labelEntry = &dns->labels[dns->no_labels];
    labelEntry->length = strlen(curname);
    memory_copy(labelEntry->name, curname, labelEntry->length);
    curname[0] = '\0';
    dns->no_labels++;  // mark final 0 label
    // memory_copy(dnsEnd, payload, payloadLength);
    dns->labels[dns->no_labels].length = 0;

    dns->no_labels++;
}

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
    generateDNSLabels(dns, payload, payloadLength);
    
    dns->query_type = 1;
    dns->query_class = 1;

    dns->is_response = 0;
    dns->length = payloadLength;
    dns->hashcode = computeHashCode(payload, payloadLength);

    // Config DNS server
    union IPAddress dnssrv;
    dnssrv.integerForm = 3232238081;

    constructUDPHeader(&dns->udp, &dnssrv, 50001, 53, &dns->id, sizeof(struct DNSPacket) - sizeof(struct UDPPacket) + payloadLength + 4 + no_label);
}

void sendDNS(struct DNSPacket* dns, char* name, uint16_t payloadLength) {
    // memory_copy((uintptr_t)(dns + 1), name, payloadLength);

    convertDNSEndianness(dns, payloadLength);

    transmit_packet(dns, sizeof(struct DNSPacket) + payloadLength + 4 + 4); // we have 4 labels, MUST compute this somehow
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

void generateDNSHeaderBytes(struct DNSPacket* dns, uintptr_t buffer) {
    generateUDPHeaderBytes(&dns->udp, buffer);
    uintptr_t bufferDNS = (uintptr_t)(buffer + ETHERNET_HEADER_LEN + IP_HEADER_LEN + UDP_HEADER_LEN + DNS_HEADER_LEN);
    *(uint16_t*)(bufferDNS) = little_to_big_endian_word(dns->id);
    *(uint16_t*)(bufferDNS + 2) = little_to_big_endian_word(dns->flags);
    *(uint16_t*)(bufferDNS + 4) = little_to_big_endian_word(dns->no_questions);
    *(uint16_t*)(bufferDNS + 6) = little_to_big_endian_word(dns->no_answer_rr);
    *(uint16_t*)(bufferDNS + 8) = little_to_big_endian_word(dns->no_authority_rr);
    *(uint16_t*)(bufferDNS + 10) = little_to_big_endian_word(dns->no_additional_rr);
    
    uintptr_t bufferPayload = (uintptr_t)(bufferDNS + 12);
    uint32_t i;
    for (i=0; i < dns->no_labels; i++) {
        *(uint8_t*)(bufferPayload) = dns->labels[i].length;
        memory_copy(bufferPayload + 1, dns->labels[i].name, dns->labels[i].length);
        bufferPayload += (dns->labels[i].length + 1);
    }

    uint16_t* queryParams = (uint16_t*)(bufferPayload);
    *queryParams = little_to_big_endian_word(dns->query_type);           // Query type
    *(queryParams + 1) = little_to_big_endian_word(dns->query_class);     // Class IN
}

// uint16_t getDNSPacketSize(struct DNSPacket* dns);
uintptr_t parseDNSPacket(uintptr_t buffer, struct DNSPacket* dns);
struct DNSPacket* pollDNS(char* dnsName);
void registerDNSEntry(DNSPacket* dns);