#ifndef _TCP_H
#define _TCP_H

#include <stdint.h>
#include "ipv4.h"

#define TCP_FLAG_FIN    1
#define TCP_FLAG_SYN    (1 << 1)
#define TCP_FLAG_RESET  (1 << 2)
#define TCP_FLAG_PUSH   (1 << 3)
#define TCP_FLAG_ACK    (1 << 4)
#define TCP_FLAG_URGENT (1 << 5)
#define TCP_FLAG_ECN    (1 << 6)
#define TCP_FLAG_CWR    (1 << 7)    // Congestion Window Reduced
#define TCP_FLAG_AECN   (1 << 8)

#define TCP_HEADER_LEN  20  // 20 bytes basic header (other options will be added separately)

struct TCPPacket {
    struct IPPacket ip;
    uint16_t srcport;
    uint16_t dstport;
    uint32_t seq;
    uint32_t ack;
    uint16_t offset_and_flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
    // uint32_t options;   // variable length options + padding
    // will only use MSS for now to limit packet size, will implement the rest when I create some more user-friendly structures and better way of handling rx/tx
    uint8_t msskey;     // temporary control keys
    uint8_t msslength;
    uint16_t mss;   // Max segment size

    // payload comes here
};

void constructTCPHeader(struct TCPPacket* tcp, union IPAddress* destip, uint16_t srcport, uint16_t dstport, uintptr_t payload, uint16_t payloadLength, uint8_t isSyn);
uint16_t calculateTCPChecksum(struct TCPPacket* tcpHeader, uintptr_t payload, uint16_t payloadLength);
void sendTCP(struct TCPPacket *tcp, uintptr_t buff, uint16_t buffLen);

void convertTCPEndianness(struct TCPPacket* tcp);

#endif
