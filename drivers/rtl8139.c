#include "rtl8139.h"
#include "../cpu/pci.h"
#include "screen.h"
#include "../libc/mem.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../libc/function.h"

#include "../libc/endian.h"
#include "arp.h"

#define TOK 0x4
#define ROK 0x1

#define RX_BUFFER_ADDR 0x003F0000   // just a bit before kmalloc start addr

#define TX_BUFFER0      0x003E0000
#define TX_BUFFER1      0x003E4000
#define TX_BUFFER2      0x003E8000
#define TX_BUFFER3      0x003EC000
#define TSA             0x20
#define TSC             0x10

#define RX_BUFFER_SIZE 8192

uint32_t RTL8139BaseAddress;
uint8_t RTL8139TXRegOffset;
uint16_t RTL8139RXOffset;

static void rtl8139_handler(registers_t*);

uint32_t identifyRTL8139() {
    // for some reason, the address may be misaligned (let's only reset the last 4 bits for now)
    uint32_t addr = getDeviceBAR0(0x10EC, 0x8139);
    RTL8139BaseAddress = (addr & 0xFFFFFFF0);
    return RTL8139BaseAddress;
}

uint8_t* getMACAddress() {
    uint8_t* buff = kmalloc(6);
    uint8_t i;

    for (i=0; i<6; i++) {
        buff[i] = port_byte_in(RTL8139BaseAddress + i);
    }

    return buff;
}

// rx_buffer must be 8k+16+1500 bytes
uint8_t initializeRTL8139() {
    kprintf("RTL8139 Init IO Addr: %u\n", RTL8139BaseAddress);
    // First check if PCI Bus Mastering is enabled
    uint16_t commandReg = pciConfigReadWord(0, 3, 0, 4);
    uint8_t enabled = (uint8_t)(commandReg & 0x4);

    if (enabled == 0) {
        kprint("RTL8139 PCI Bus Mastering is DISABLED. Enabling now...\n");
        // we MUST enable PCI Bus Mastering
        uint8_t mask = commandReg |= (1 << 2);     // a fancier way of writing 4
        pciConfigWriteWord(0, 3, 0, 4, mask);

        // check value
        commandReg = pciConfigReadWord(0, 3, 0, 4);
        enabled = (uint8_t)(commandReg & 0x4);
    }

    kprintf("PCI Bus Mastering is %s\n", enabled > 0 ? "ENABLED" : "DISABLED");

    // Power on the device
    port_byte_out( RTL8139BaseAddress + 0x52, 0x0);

    kprint("RTL8139 Powered On\n");

    // Software reset
    uint8_t resetCommand;
    port_byte_out( RTL8139BaseAddress + 0x37, 0x10);
    while( ((resetCommand = port_byte_in(RTL8139BaseAddress + 0x37)) & 0x10) != 0) { kprintf("Reset Comm = %x\n", resetCommand); }

    kprint("RTL Software Reset done!\n");

    // Set Receive buffer
    port_dword_out(RTL8139BaseAddress + 0x30, (uintptr_t)RX_BUFFER_ADDR); // send uint32_t memory location to RBSTART (0x30)

    // Allow only TOK and ROK IRQ events
    port_word_out(RTL8139BaseAddress + 0x3C, 0xFFFF); // Sets the TOK and ROK bits high

    // Configure Receiver buffer
    port_dword_out(RTL8139BaseAddress + 0x44, 0xf | (1 << 7)); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

    // Enabled RX and TX
    port_byte_out(RTL8139BaseAddress + 0x37, 0x0C); // Sets the RE and TE bits high

    // Enable IRQ callback
    uint8_t rtlIRQ = getIRQNumber(0x10EC, 0x8139);
    kprintf("RTL IRQ Number: %u\n", rtlIRQ);

    register_interrupt_handler(rtlIRQ + 32, rtl8139_handler);   // our IRQ handlers are shifted by 32 positions (IRQ0=32), may be subject to change?

    return 0;
}

void transmit_packet(void* buffer, uint16_t bufLenth) {
    // not sure if I should also check some TX status before transmitting

    port_dword_out(RTL8139BaseAddress + TSA + RTL8139TXRegOffset * 4, buffer);
    port_dword_out(RTL8139BaseAddress + TSC + RTL8139TXRegOffset * 4, bufLenth);
    RTL8139TXRegOffset = (RTL8139TXRegOffset + 1) % 4;
}

static void receive_packet() {
    // Read the contents of the RX buffer (hopefully this is where the data is stored)
    uint8_t* buff = (uint8_t*) (RX_BUFFER_ADDR + RTL8139RXOffset);

    // Skip packet header ??

    // Read packet len
    uint16_t frameLen = *((uint16_t*)buff + 1);
    kprintf("Frame length: %u\n", frameLen);

    buff += 4;

    uint16_t ethType = big_to_little_endian_word(((struct EthernetFrame*)(buff))->ethtype);

    switch (ethType) {
        case 0x0806:
            kprint("Received Eth Type: ARP\n");
            struct ARP* arp = (struct ARP*) buff;
            kprintf("ARP Reply Opcode: %x\n", arp->opcode);
            kprintf("ARP Reply MAC Addr: %x:%x:%x:%x:%x:%x\n", arp->srchw[0], arp->srchw[1], arp->srchw[2], arp->srchw[3], arp->srchw[4], arp->srchw[5]);
            break;
        default:
            kprint("Received Eth Type: I don't know\n");
    }

    kprintf("Received packet first bytes: %x %x %x %x %x\n", buff[0], buff[1], buff[2], buff[3], buff[4]);

    // Should make a copy of the frame

    // Update packet offset
    RTL8139RXOffset = RTL8139RXOffset + frameLen + 4 + 3;

    // the buffer has spare bytes after end, so strict inequality can be used
    if (RTL8139RXOffset > RX_BUFFER_SIZE)
        RTL8139RXOffset -= RX_BUFFER_SIZE;

    // Set RX Buffer offset
    port_word_out(RTL8139BaseAddress + 0x38, RTL8139RXOffset - 0x10);
}

static void rtl8139_handler(registers_t* regs) {
    kprint("GOT RTL8139 IRQ!!!!\n");
	uint16_t status = port_word_in(RTL8139BaseAddress + 0x3e);
	port_word_out(RTL8139BaseAddress + 0x3E, 0x05);
	if(status & TOK) {
		// Sent
        kprint("Successfully sent NET packet");
        // RTL8139TXRegOffset = (RTL8139TXRegOffset + 1) % 4;
	}
	if (status & ROK) {
		// Received
		receive_packet();
	}

    UNUSED(regs);
}
