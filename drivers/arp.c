#include "arp.h"
#include "../libc/mem.h"
#include "../libc/endian.h"

#include "ethernet.h"
#include "../libc/function.h"

#define ARP_CACHE_MAX_SIZE 128

static struct ARPEntry ArpCache[ARP_CACHE_MAX_SIZE];
static uint32_t ArpCacheIndex = 0;

struct ARP* constructARP(union IPAddress* addr) {
    struct ARP* arp = (struct ARP*) kmalloc(sizeof(struct ARP));

    constructEthernetBroadcast(&arp->eth);  // for now, make it a broadcast ARP

    arp->htype = little_to_big_endian_word(0x1);   // Ethernet
    arp->ptype = little_to_big_endian_word(0x0800);    // IP
    arp->hlen = 6;
    arp->plen = 4;
    arp->opcode = little_to_big_endian_word(1);    // 1=Request, 2=Reply
    memory_copy(arp->srchw, getMACAddress(), 6);    // should free mac address memory
    memory_copy(arp->srcpr, getDummyIP(), 4);       // we will emulate IP for now

    // dsthw is ignored in ARP
    memory_copy(arp->dstpr, addr->bytes, 4);

    return arp;
}

void sendARP(struct ARP* arp) {
    transmit_packet(arp, sizeof(struct ARP));
}

uint8_t* getARPEntry(union IPAddress* addr) {
    uint32_t i;

    for (i=0; i < ARP_CACHE_MAX_SIZE && ArpCache[i].ip.integerForm != 0; i++) {
        if (ArpCache[i].ip.integerForm == addr->integerForm)
            break;
    }

    if (i >= ARP_CACHE_MAX_SIZE || ArpCache[i].ip.integerForm == 0) {
        // we reached the end of the list
        return NULL;
    }

    // it may get overwritten in time. A copy MUST be made for this
    return ArpCache[i].mac;
}

void addARPEntry(union IPAddress* ip, uint8_t* mac) {
    ArpCache[ArpCacheIndex].ip.integerForm = ip->integerForm;
    memory_copy(ArpCache[ArpCacheIndex].mac, mac, 6);
    ArpCacheIndex = (ArpCacheIndex + 1) % ARP_CACHE_MAX_SIZE;
}
