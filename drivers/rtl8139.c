#include "rtl8139.h"
#include "../cpu/pci.h"
#include "screen.h"
#include "../libc/mem.h"
#include "../cpu/ports.h"

uint32_t identifyRTL8139() {
    // for some reason, the address may be misaligned (let's only reset the last 4 bits for now)
    uint32_t addr = getDeviceBAR0(0x10EC, 0x8139);
    return (addr & 0xFFFFFFF0);
}

uint8_t* getMACAddress(uint32_t baseAddress) {
    uint8_t* buff = kmalloc(6);
    uint8_t i;

    for (i=0; i<6; i++) {
        buff[i] = port_byte_in(baseAddress + i);
    }

    return buff;
}

// rx_buffer must be 8k+16+1500 bytes
uint8_t initializeRTL8139(uint32_t baseAddress, uint8_t* rx_buffer) {
    // First check if PCI Bus Mastering is enabled
    uint16_t commandReg = pciConfigReadWord(0, 3, 0, 4);
    uint8_t enabled = (uint8_t)(commandReg & 0x2);

    // the Master Bit can be enabled, but not right now, not in the mood :))
    kprintf("PCI Bus Mastering is %s\n", enabled > 0 ? "ENABLED" : "DISABLED");

    // Power on the device
    port_byte_out( baseAddress + 0x52, 0x0);

    // Software reset
    port_byte_out( baseAddress + 0x37, 0x10);
    while( (port_byte_in(baseAddress + 0x37) & 0x10) != 0) { }

    kprint("RTL Software Reset done!\n");

    // Set Receive buffer
    port_dword_out(baseAddress + 0x30, (uintptr_t)rx_buffer); // send uint32_t memory location to RBSTART (0x30)

    // Allow only TOK and ROK IRQ events
    port_dword_out(baseAddress + 0x3C, 0x0005); // Sets the TOK and ROK bits high

    // Configure Receiver buffer
    port_dword_out(baseAddress + 0x44, 0xf | (1 << 7)); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

    // Enabled RX and TX
    port_byte_out(baseAddress + 0x37, 0x0C); // Sets the RE and TE bits high

    return 0;
}
