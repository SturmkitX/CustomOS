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
    memory_copy(arp->srcpr, getIPAddress()->bytes, 4);       // we will emulate IP for now

    // dsthw is ignored in ARP
    memory_copy(arp->dstpr, dstaddr->bytes, 4);

    // initialize padding
    uint32_t i;
    for (i=0; i<18; i++)
        arp->padding[i] = 0;
}

void sendARP(struct ARP* arp, union IPAddress* ip) {
    ArpCache[ArpCacheIndex].ip.integerForm = ip->integerForm;

    transmit_packet(arp, sizeof(struct ARP));
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

    union IPAddress* srcip = (union IPAddress*)arp->srcpr;
    union IPAddress* dstip = (union IPAddress*)arp->dstpr;

    memory_copy(arp->srcpr, ((union IPAddress)(little_to_big_endian_dword(srcip->integerForm))).bytes, 4);       // we will emulate IP for now
    memory_copy(arp->dstpr, ((union IPAddress)(little_to_big_endian_dword(dstip->integerForm))).bytes, 4);

    // also convert downwards
    convertEthernetFrameEndianness(&arp->eth);
}
