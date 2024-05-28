#include "rtl8139.h"
#include "../cpu/pci.h"
#include "screen.h"
#include "../libc/mem.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../libc/function.h"

#define TOK 0x4
#define ROK 0x1

#define RX_BUFFER_ADDR 0x003F0000   // just a bit before kmalloc start addr

#define TX_BUFFER0      0x003E0000
#define TX_BUFFER1      0x003E0004
#define TX_BUFFER2      0x003E0008
#define TX_BUFFER3      0x003E000C
#define TSA             0x20
#define TSC             0x10

uint32_t RTL8139BaseAddress;
uint8_t RTL8139TXRegOffset;

static void rtl8139_handler(registers_t*);

uint32_t identifyRTL8139() {
    // for some reason, the address may be misaligned (let's only reset the last 4 bits for now)
    uint32_t addr = getDeviceBAR0(0x10EC, 0x8139);
    RTL8139BaseAddress = addr;
    return (addr & 0xFFFFFFF0);
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
    // First check if PCI Bus Mastering is enabled
    uint16_t commandReg = pciConfigReadWord(0, 3, 0, 4);
    uint8_t enabled = (uint8_t)(commandReg & 0x2);

    // the Master Bit can be enabled, but not right now, not in the mood :))
    kprintf("PCI Bus Mastering is %s\n", enabled > 0 ? "ENABLED" : "DISABLED");

    // Power on the device
    port_byte_out( RTL8139BaseAddress + 0x52, 0x0);

    // Software reset
    port_byte_out( RTL8139BaseAddress + 0x37, 0x10);
    while( (port_byte_in(RTL8139BaseAddress + 0x37) & 0x10) != 0) { }

    kprint("RTL Software Reset done!\n");

    // Set Receive buffer
    port_dword_out(RTL8139BaseAddress + 0x30, (uintptr_t)RX_BUFFER_ADDR); // send uint32_t memory location to RBSTART (0x30)

    // Allow only TOK and ROK IRQ events
    port_dword_out(RTL8139BaseAddress + 0x3C, 0x0005); // Sets the TOK and ROK bits high

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
    port_dword_out(RTL8139BaseAddress + TSA + RTL8139TXRegOffset * 4, buffer);
    port_dword_out(RTL8139BaseAddress + TSC + RTL8139TXRegOffset * 4, (bufLenth & 0x1FFF));
}

static void receive_packet() {
    // Read the contents of the RX buffer (hopefully this is where the data is stored)
    uint8_t* buff = (uint8_t*) RX_BUFFER_ADDR;
    kprintf("Received packet first bytes: %x %x %x\n", buff[0], buff[1], buff[2]);
}

static void rtl8139_handler(registers_t* regs) {
	uint16_t status = port_word_in(RTL8139BaseAddress + 0x3e);
	port_word_out(RTL8139BaseAddress + 0x3E, 0x05);
	if(status & TOK) {
		// Sent
        kprint("Successfully send NET packet");
        RTL8139TXRegOffset = (RTL8139TXRegOffset + 1) % 4;
	}
	if (status & ROK) {
		// Received
		receive_packet();
	}

    UNUSED(regs);
}
