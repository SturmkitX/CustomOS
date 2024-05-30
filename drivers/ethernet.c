#include "ethernet.h"

#include "../libc/mem.h"
#include "../libc/endian.h"

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
