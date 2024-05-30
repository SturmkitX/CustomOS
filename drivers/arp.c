#include "arp.h"
#include "../libc/mem.h"
#include "../libc/endian.h"
#include "rtl8139.h"

uint8_t* getDummyIP() {
    union IPAddress* ip = (union IPAddress*) kmalloc(sizeof(union IPAddress));
    ip->integerForm = low_to_big_endian_dword(167772687);    // 10.0.2.15

    return ip->bytes;
}

void constructARPInternal(union IPAddress* addr, struct ARP* arp) {
    arp->htype = low_to_big_endian_word(0x1);   // Ethernet
    arp->ptype = low_to_big_endian_word(0x0800);    // IP
    arp->hlen = 6;
    arp->plen = 4;
    arp->opcode = low_to_big_endian_word(1);    // 1=Request, 2=Reply
    memory_copy(arp->srchw, getMACAddress(), 6);    // should free mac address memory
    memory_copy(arp->srcpr, getDummyIP(), 4);       // we will emulate IP for now

    // dsthw is ignored in ARP
    memory_copy(arp->dstpr, addr->bytes, 4);
}

struct ARP* constructARP(union IPAddress* addr) {
    struct ARP* arp = (struct ARP*) kmalloc(sizeof(struct ARP));

    constructARPInternal(addr, arp);

    return arp;
}

struct EthARP* constructEthARP(union IPAddress* addr) {
    struct EthARP* eth = (struct EthARP*) kmalloc(sizeof(struct EthARP));
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    memory_copy(eth->srchw, getMACAddress(), 6);
    memory_copy(eth->dsthw, broadcast, 6);
    eth->ethtype = low_to_big_endian_word(0x0806);  // ARP

    constructARPInternal(addr, &eth->arp);

    return eth;
}