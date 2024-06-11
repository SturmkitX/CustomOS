#include "ethernet.h"

#include "../libc/mem.h"
#include "../libc/endian.h"
#include "ipv4.h"
#include "arp.h"

uint32_t _internal_ip_addr = 167772687;         // 10.0.2.15
uint32_t _internal_subnet_mask = 0xFFFFFF00;    // 255.255.255.0 ??
uint32_t _internal_gateway = 167772674;         // 10.0.2.2

union IPAddress* getIPAddress() {
    return (union IPAddress*)(&_internal_ip_addr);
}

uint32_t getSubnetMask() {
    return _internal_subnet_mask;
}

union IPAddress* getGateway() {
    return (union IPAddress*)(&_internal_gateway);
}

void constructEthernetBroadcast(struct EthernetFrame* eth, uint16_t ethtype) {
    uint32_t i;

    memory_copy(eth->srchw, getMACAddress(), 6);

    for (i=0; i < 6; i++)
        eth->dsthw[i] = 0xFF;
    eth->ethtype = ethtype;
}

void constructEthernetFrame(struct EthernetFrame* eth, uint8_t* destMAC, uint16_t ethtype) {
    uint32_t i;

    memory_copy(eth->srchw, getMACAddress(), 6);
    memory_copy(eth->dsthw, destMAC, 6);
    eth->ethtype = ethtype;
}

void convertEthernetFrameEndianness(struct EthernetFrame* eth) {
    eth->ethtype = little_to_big_endian_word(eth->ethtype);
}

void generateEthernetFrameBytes(struct EthernetFrame* eth, uintptr_t buffer) {
    memory_copy(buffer, eth->dsthw, 6);
    memory_copy(buffer + 6, eth->srchw, 6);
    *(uint16_t*)(buffer + 12) = little_to_big_endian_word(eth->ethtype);
}

uintptr_t parseEthernetFrame(uintptr_t buffer, struct EthernetFrame* eth) {
    memory_copy(eth->dsthw, buffer, 6);
    memory_copy(eth->srchw, buffer + 6, 6);
    eth->ethtype = big_to_little_endian_word(*(uint16_t*)(buffer + 12));

    return (buffer + ETHERNET_HEADER_LEN);
}

void handleEthernetFrameRecv(uintptr_t buffer) {
    struct EthernetFrame eth;
    uintptr_t remainingBuff = parseEthernetFrame(buffer, &eth);
    switch (eth.ethtype) {
        case 0x0806:
            kprint("Received Eth Type: ARP\n");
            struct ARP arp;
            memory_copy(&arp.eth, &eth, sizeof(eth));
            remainingBuff = parseARPHeader(remainingBuff, &arp);
            associateMACAddress(arp.srchw);
            kprintf("ARP Reply Opcode: %x\n", arp.opcode);
            kprintf("ARP Reply MAC Addr: %x:%x:%x:%x:%x:%x\n", arp.srchw[0], arp.srchw[1], arp.srchw[2], arp.srchw[3], arp.srchw[4], arp.srchw[5]);
            break;
        case 0x0800:
            kprint("Received IPv4 packet\n");
            handleIPPacketRecv(remainingBuff, &eth);
            break;
        default:
            kprint("Received Eth Type: I don't know\n");
    }
}
