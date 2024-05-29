#include "arp.h"
#include "../libc/mem.h"
#include "rtl8139.h"

uint8_t* getDummyIP() {
    union IPAddress* ip = (union IPAddress*) kmalloc(sizeof(union IPAddress));
    ip->integerForm = 167772687;    // 10.0.2.15

    return ip->bytes;
}

struct ARP* constructARP(union IPAddress* addr) {
    struct ARP* arp = (struct ARP*) kmalloc(sizeof(struct ARP));

    arp->htype = 0x1;   // Ethernet
    arp->ptype = 0x0800;    // IP
    arp->hlen = 6;
    arp->plen = 4;
    arp->opcode = 1;    // 1=Request, 2=Reply
    memory_copy(arp->srchw, getMACAddress(), 6);    // should free mac address memory
    memory_copy(arp->srcpr, getDummyIP(), 4);       // we will emulate IP for now

    // dsthw is ignored in ARP
    memory_copy(arp->dstpr, addr->bytes, 4);

    return arp;
}