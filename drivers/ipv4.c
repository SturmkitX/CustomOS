#include "ipv4.h"
#include "../libc/mem.h"
#include "../libc/endian.h"
#include "arp.h"
#include "../drivers/screen.h"

// Implementation also does the Endianness conversion

static uint16_t CurrentIdentification = 1;

// IPHeader MUST HAVE ALL ITS FIELDS IN LITTLE ENDIAN (because that is what x86 uses)
// odd sized payloads WILL PROBABLY ADD a bad byte at the end
uint32_t calculateIPHeaderChecksumPhase1(struct IPPacket* ipHeader) {
    uint32_t sum = 0;

    sum += ((ipHeader->version_header << 8) | ipHeader->tos);
    sum += ipHeader->total_length;
    sum += ipHeader->identification;
    sum += ipHeader->flags_fragment_offset;
    sum += ((ipHeader->ttl << 8) | ipHeader->protocol);
    
    sum += (ipHeader->srcip.integerForm >> 16);
    sum += (ipHeader->srcip.integerForm & 0xFFFF);

    sum += (ipHeader->dstip.integerForm >> 16);
    sum += (ipHeader->dstip.integerForm & 0xFFFF);

    // kprintf("IPv4 Check Partial sum: %x. Members:\n", sum);
    // kprintf("%x %x %x %x %x %x %x %x %x\n", ((ipHeader->version_header << 8) | ipHeader->tos),
    //     ipHeader->total_length,
    //     ipHeader->identification,
    //     ipHeader->flags_fragment_offset,
    //     ((ipHeader->ttl << 8) | ipHeader->protocol),
    //     ((ipHeader->srcip[3] << 8) | ipHeader->srcip[2]),
    //     ((ipHeader->srcip[1] << 8) | ipHeader->srcip[0]),
    //     ((ipHeader->dstip[3] << 8) | ipHeader->dstip[2]),
    //     ((ipHeader->dstip[1] << 8) | ipHeader->dstip[0])
    // );

    return sum;
}
uint16_t wrapHeaderChecksum(uint32_t sum) {
    if (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);     // generally, it is considered that one pass should be enough
    }

    return (uint16_t)(~sum);
}

// just a wrapper for the previous 2 functions
uint16_t calculateIPHeaderChecksum(struct IPPacket* ipHeader) {
    uint32_t sum = calculateIPHeaderChecksumPhase1(ipHeader);
    return wrapHeaderChecksum(sum);
}

uint16_t calculateIPChecksum(struct IPPacket* ipHeader) {
    return calculateIPHeaderChecksum(ipHeader);
}

void constructIPPacket(struct IPPacket* ip, uint16_t payload_len, uint8_t protocol, union IPAddress* dstip) {
    ip->version_header = 0x45;      // IPv4, 20 bytes long header (5 dwords)
    ip->tos = 0;                    // Nothing special wanted right now
    ip->total_length = IP_HEADER_LEN + payload_len;    // For a 20 bytes header
    ip->identification = CurrentIdentification++;
    ip->flags_fragment_offset = 0;  // don't think we need it now
    ip->ttl = 128;  // It's ok for now
    ip->protocol = protocol;

    ip->srcip.integerForm = getIPAddress()->integerForm;
    ip->dstip.integerForm = dstip->integerForm;

    ip->header_checksum = calculateIPChecksum(ip);

    // we still have to construct the Ethernet Frame
    // we need to get the ARP entry
    union IPAddress* ip_to_send = dstip;

    // Printing entire IP header for header check
    // uint32_t i;
    // for (i=0; i < 20; i++)
    //     kprintf("%x ", ((uint8_t*)(&ip->version_header))[i]);

    // check if address is private
    if ((dstip->integerForm & getSubnetMask()) != (getIPAddress()->integerForm & getSubnetMask())) {
        ip_to_send = getGateway();
        kprintf("IPV4: Dest Addr is NOT private; Setting Dest MAC to Gateway: %u.%u.%u.%u\n", ip_to_send->bytes[3], ip_to_send->bytes[2], ip_to_send->bytes[1], ip_to_send->bytes[0]);
    }

    uint8_t* dstMac = getARPEntry(ip_to_send);
    kprint("Got first MAC\n");
    if (dstMac == NULL) {
        kprint("First MAC is NULL\n");
        // request the MAC address
        struct ARP arp;
        constructARP(&arp, ip_to_send);
        // convertARPEndianness(&arp);
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

void convertIPPacketEndianness(struct IPPacket* ip) {
    ip->total_length = little_to_big_endian_word(ip->total_length);
    ip->identification = little_to_big_endian_word(ip->identification);

    ip->srcip.integerForm = little_to_big_endian_dword(ip->srcip.integerForm);
    ip->dstip.integerForm = little_to_big_endian_dword(ip->dstip.integerForm);

    ip->header_checksum = little_to_big_endian_word(ip->header_checksum);

    convertEthernetFrameEndianness(&ip->eth);
}

void generateIPHeaderBytes(struct IPPacket* ip, uintptr_t buffer) {
    generateEthernetFrameBytes(&ip->eth, buffer);
    uintptr_t bufferIP = (uintptr_t)(buffer + ETHERNET_HEADER_LEN);

    *(uint8_t*)bufferIP = ip->version_header;
    *(uint8_t*)(bufferIP + 1) = ip->tos;
    *(uint16_t*)(bufferIP + 2) = little_to_big_endian_word(ip->total_length);  // Header + Payload
    *(uint16_t*)(bufferIP + 4) = little_to_big_endian_word(ip->identification);
    *(uint16_t*)(bufferIP + 6) = little_to_big_endian_word(ip->flags_fragment_offset);
    *(uint8_t*)(bufferIP + 8) = ip->ttl;
    *(uint8_t*)(bufferIP + 9) = ip->protocol;
    *(uint16_t*)(bufferIP + 10) = little_to_big_endian_word(ip->header_checksum);
    *(uint32_t*)(bufferIP + 12) = little_to_big_endian_dword(ip->srcip.integerForm);
    *(uint32_t*)(bufferIP + 16) = little_to_big_endian_dword(ip->dstip.integerForm);
}
