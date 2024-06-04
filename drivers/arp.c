#include "arp.h"
#include "../libc/mem.h"
#include "../libc/endian.h"

#include "ethernet.h"
#include "../libc/function.h"

#define ARP_CACHE_MAX_SIZE 128

struct ARPEntry ArpCache[ARP_CACHE_MAX_SIZE];
uint32_t ArpCacheIndex = 0;

void constructARP(struct ARP* arp, union IPAddress* dstaddr) {
    constructEthernetBroadcast(&arp->eth, 0x0806);  // for now, make it a broadcast ARP

    arp->htype = 0x1;   // Ethernet
    arp->ptype = 0x0800;    // IP
    arp->hlen = 6;
    arp->plen = 4;
    arp->opcode = 1;    // 1=Request, 2=Reply
    memory_copy(arp->srchw, getMACAddress(), 6);    // should free mac address memory
    arp->srcpr.integerForm = getIPAddress()->integerForm;       // we will use a static IP for now

    // dsthw is ignored in ARP
    arp->dstpr.integerForm = dstaddr->integerForm;

    // initialize padding
    uint32_t i;
    for (i=0; i<18; i++)
        arp->padding[i] = 0;
}

void sendARP(struct ARP* arp, union IPAddress* ip) {
    ArpCache[ArpCacheIndex].ip.integerForm = ip->integerForm;

    uint8_t arpBytes[64];
    generateARPHeaderBytes(arp, arpBytes);

    transmit_packet(arpBytes, 60);  // we kinda have a fixed size of 60 bytes
    // the next ARP reply will be associated with this one (since the original IP is lost during transmission)
    // Index will be incremented after saving the mac (if found)
}

uint8_t* getARPEntry(union IPAddress* addr) {
    uint32_t i;
    // kprint("Reached get ARP Entry\n");

    for (i=0; i < ARP_CACHE_MAX_SIZE && ArpCache[i].ip.integerForm != 0; i++) {
        if (ArpCache[i].ip.integerForm == addr->integerForm && ArpCacheIndex != i)  // ArpCacheIndex == i means we are still waiting for ARP result
            break;
    }

    if (i >= ARP_CACHE_MAX_SIZE || ArpCache[i].ip.integerForm == 0) {
        // we reached the end of the list
        return NULL;
    }

    // it may get overwritten in time. A copy MUST be made for this
    return (uint8_t*) ArpCache[i].mac;
}

void associateMACAddress(uint8_t* mac) {
    // should check for invalid MAC addresses (when I will find one)
    memory_copy(ArpCache[ArpCacheIndex].mac, mac, 6);
    ArpCacheIndex++;
}

void convertARPEndianness(struct ARP* arp) {
    arp->htype = little_to_big_endian_word(arp->htype);
    arp->ptype = little_to_big_endian_word(arp->ptype);
    arp->opcode = little_to_big_endian_word(arp->opcode);

    arp->srcpr.integerForm = little_to_big_endian_dword(arp->srcpr.integerForm);       // we will emulate IP for now
    arp->dstpr.integerForm = little_to_big_endian_dword(arp->dstpr.integerForm);

    // also convert downwards
    convertEthernetFrameEndianness(&arp->eth);
}

void generateARPHeaderBytes(struct ARP* arp, uintptr_t buffer) {
    generateEthernetFrameBytes(&arp->eth, buffer);
    uintptr_t bufferARP = (uintptr_t)(buffer + ETHERNET_HEADER_LEN);

    *(uint16_t*)(bufferARP) = little_to_big_endian_word(arp->htype); // Hardware type
    *(uint16_t*)(bufferARP + 2) = little_to_big_endian_word(arp->ptype); // Protocol type
    *(uint8_t*)(bufferARP + 4) = arp->hlen; // Hardware address length (Ethernet = 6)
    *(uint8_t*)(bufferARP + 5) = arp->plen; // Protocol address length (IPv4 = 4)
    *(uint16_t*)(bufferARP + 6) = little_to_big_endian_word(arp->opcode); // ARP Operation Code
    memory_copy(bufferARP + 8, arp->srchw, 6); // Source hardware address - hlen bytes (see above)
    *(uint32_t*)(bufferARP + 14) = little_to_big_endian_dword(arp->srcpr.integerForm); // Source protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
    memory_copy(bufferARP + 18, arp->dsthw, 6); // Destination hardware address - hlen bytes (see above)
    *(uint32_t*)(bufferARP + 24) = little_to_big_endian_dword(arp->dstpr.integerForm); // Destination protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
    memory_copy(bufferARP + 28, arp->padding, 18);   // Padding to get minimum size for Ethernet packets (64 bytes)
}
