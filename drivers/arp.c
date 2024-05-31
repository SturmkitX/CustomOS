#include "arp.h"
#include "../libc/mem.h"
#include "../libc/endian.h"

#include "ethernet.h"
#include "../libc/function.h"

#define ARP_CACHE_MAX_SIZE 128

static struct ARPEntry ArpCache[ARP_CACHE_MAX_SIZE];
static uint32_t ArpCacheIndex = 0;

void constructARP(struct ARP* arp, union IPAddress* dstaddr) {
    constructEthernetBroadcast(&arp->eth, 0x0806);  // for now, make it a broadcast ARP

    union IPAddress my_ip_be;
    my_ip_be.integerForm = little_to_big_endian_dword(getIPAddress()->integerForm);

    arp->htype = little_to_big_endian_word(0x1);   // Ethernet
    arp->ptype = little_to_big_endian_word(0x0800);    // IP
    arp->hlen = 6;
    arp->plen = 4;
    arp->opcode = little_to_big_endian_word(1);    // 1=Request, 2=Reply
    memory_copy(arp->srchw, getMACAddress(), 6);    // should free mac address memory
    memory_copy(arp->srcpr, my_ip_be.bytes, 4);       // we will emulate IP for now

    // dsthw is ignored in ARP
    memory_copy(arp->dstpr, ((union IPAddress)(little_to_big_endian_dword(dstaddr->integerForm))).bytes, 4);
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
