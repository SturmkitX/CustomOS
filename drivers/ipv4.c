#include "ipv4.h"
#include "../libc/mem.h"
#include "../libc/endian.h"
#include "arp.h"
#include "../drivers/screen.h"

// Implementation also does the Endianness conversion

static uint16_t CurrentIdentification = 1;

uint16_t calculateHeaderChecksum(struct IPPacket* ipHeader, uint16_t len, uint16_t* buff, uintptr_t checksumAddr) {
    uint16_t i;
    uint32_t sum = 0;

    for(i=0; i < len/2; i++) {
        if (buff + i == checksumAddr)     // skip the checksum itself
            continue;

        sum += buff[i];
    }

    if (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);     // generally, it is considered that one pass should be enough
    }

    return (uint16_t)(~sum);
}

uint16_t calculateIPChecksum(struct IPPacket* ipHeader) {
    return calculateHeaderChecksum(ipHeader, IP_HEADER_LEN, (uint16_t*)(&ipHeader->version_header), &ipHeader->header_checksum);
}

void constructIPPacket(struct IPPacket* ip, uint16_t payload_len, uint8_t protocol, union IPAddress* dstip) {
    ip->version_header = 0x45;      // IPv4, 20 bytes long header (5 dwords)
    ip->tos = 0;                    // Nothing special wanted right now
    ip->total_length = little_to_big_endian_word(20 + payload_len);    // For a 20 bytes header
    ip->identification = little_to_big_endian_word(CurrentIdentification++);
    ip->flags_fragment_offset = 0;  // don't think we need it now
    ip->ttl = 128;  // It's ok for now
    ip->protocol = protocol;

    memory_copy(ip->srcip, ((union IPAddress)(little_to_big_endian_dword(getIPAddress()->integerForm))).bytes, 4);
    memory_copy(ip->dstip, ((union IPAddress)(little_to_big_endian_dword(dstip->integerForm))).bytes, 4);

    ip->header_checksum = calculateIPChecksum(ip);

    // we still have to construct the Ethernet Frame
    // we need to get the ARP entry
    union IPAddress* ip_to_send = dstip;

    // check if address is private
    if ((dstip->integerForm & getSubnetMask()) != (getIPAddress()->integerForm & getSubnetMask())) {
        ip_to_send = getGateway();
        kprintf("IPV4: Dest Addr is NOT private; Setting Dest MAC to Gateway: %u:%u:%u:%u\n", ip_to_send->bytes[3], ip_to_send->bytes[2], ip_to_send->bytes[1], ip_to_send->bytes[0]);
    }

    uint8_t* dstMac = getARPEntry(ip_to_send);
    kprint("Got first MAC\n");
    if (dstMac == NULL) {
        kprint("First MAC is NULL\n");
        // request the MAC address
        struct ARP arp;
        constructARP(&arp, ip_to_send);
        sendARP(&arp, ip_to_send);

        kprint("Sent First ARP\n");

        while (dstMac == NULL) {
            // should probably add some timeout mechanism
            dstMac = getARPEntry(ip_to_send);
        }

        kprintf("IPV4: Found ARP entry: %x %x %x\n", dstMac[0], dstMac[1], dstMac[2]);
    }
    constructEthernetFrame(&ip->eth, dstMac, 0x0800);
}
