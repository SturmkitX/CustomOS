#include "arp.h"
#include "../libc/mem.h"
#include "../libc/endian.h"
#include "rtl8139.h"

uint8_t* getDummyIP() {
    union IPAddress* ip = (union IPAddress*) kmalloc(sizeof(union IPAddress));
    ip->integerForm = little_to_big_endian_dword(167772687);    // 10.0.2.15

    return ip->bytes;
}

void constructEthernetBroadcast(struct EthernetFrame* eth) {
    uint32_t i;

    memory_copy(eth->srchw, getMACAddress(), 6);

    for (i=0; i < 6; i++)
        eth->dsthw[i] = 0xFF;
    eth->ethtype = little_to_big_endian_word(0x0806);  // ARP
}

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

struct ARP* sendARP(struct ARP* arp) {
    //
}
