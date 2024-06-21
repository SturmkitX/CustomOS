#include "e1000.h"

#include "screen.h"
#include "../cpu/pci.h"
#include "../cpu/mmio.h"
#include "../cpu/timer.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../libc/function.h"

#define E1000_MMIO_START    0x00400000

// E82545EM-A (Copper, Server)
#define E82545EM_VENDORID   0x8086
#define E82545EM_DEVICEID   0x100F

#define E1000_REG_CTRL          0x00
#define E1000_REG_EECD          0x10
#define E1000_REG_EERD          0x14

#define E1000_REG_CTRL_RESET    (1 << 26)
#define E1000_REG_EECD_EE_PRES  (1 << 8)
#define E1000_REG_EECD_EE_REQ   (1 << 6)
#define E1000_REG_EECD_EE_GNT  (1 << 7)
#define E1000_REG_EERD_START    (1 << 0)
#define E1000_REG_EERD_DONE     (1 << 4)

static struct PCIAddressInfo _pci_address;

static void E1000_handler(registers_t reg);

uint8_t identifyE1000() {
    // probe for E82545EM
    getDeviceInfo(E82545EM_VENDORID, E82545EM_DEVICEID, &_pci_address);
    if (_pci_address.vendor_id != 0xFFFF && _pci_address.device_id != 0xFFFF) {
        kprint("Detected E1000 Network Card (E82545EM-A, For servers, Copper Media)\n");
        kprintf("E1000 BAR0 (MMIO address): %x\n", _pci_address.BAR0);
        kprintf("E1000 BAR1 (I/O address): %x\n", _pci_address.BAR1);

        // Replace BAR0 address with ours
        // uint32_t address = (uint32_t)((_pci_address.bus << 16) | (_pci_address.device << 11) | ((uint32_t)0x80000000));
        // port_dword_out(0xCF8, (address | 0x10));
        // port_dword_out(0xCFC, E1000_MMIO_START);

        // kprint("Reconfigured E1000 address space\n");

        return 1;
    } else {
        kprint("No E1000 card was found\n");
    }

    return 0;
}

static uint16_t i8254x_read_eeprom(uint8_t addr) {
    uint32_t tmp;
    uint16_t data;
    uint32_t E1000BaseAddress = _pci_address.BAR0;

    if((mmio_dword_in(E1000BaseAddress + E1000_REG_EECD) & E1000_REG_EECD_EE_PRES) == 0) {
        kprint("EEPROM present bit is not set for i8254x\n");
        asm("hlt");
    }

    /* Tell the EEPROM to start reading */
    tmp = ((uint32_t)addr & 0xff) << 8;
    tmp |= E1000_REG_EERD_START;
    mmio_dword_out(E1000BaseAddress + E1000_REG_EERD, tmp);

    /* Wait until the read is finished - then the DONE bit is cleared */
    while ((mmio_dword_in(E1000BaseAddress + E1000_REG_EERD) & E1000_REG_EERD_DONE) == 0) {kprint("Waiting for EEPROM...\n");}

    /* Obtain the data */
    data = (uint16_t)(mmio_dword_in(E1000BaseAddress + E1000_REG_EERD) >> 16);

    /* Tell EEPROM to stop reading */
    tmp = mmio_dword_in(E1000BaseAddress + E1000_REG_EERD);
    tmp &= ~(uint32_t)E1000_REG_EERD_START;
    mmio_dword_out(E1000BaseAddress + E1000_REG_EERD, tmp);

    return data;
}

uint8_t initializeE1000() {
    uint32_t E1000BaseAddress = _pci_address.BAR0;

    // First check if PCI Bus Mastering is enabled
    uint16_t commandReg = pciConfigReadWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4);
    uint8_t enabled = (uint8_t)(commandReg & 0x7);

    if (enabled != 7) {
        kprint("E1000 PCI Bus Mastering is DISABLED. Enabling now...\n");
        // we MUST enable PCI Bus Mastering
        uint8_t mask = commandReg |= 7;     // a fancier way of writing 4
        pciConfigWriteWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4, mask);

        // check value
        commandReg = pciConfigReadWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4);
        enabled = (uint8_t)(commandReg & 0x7);
    }

    kprintf("PCI Bus Mastering is %s\n", enabled == 7 ? "ENABLED" : "DISABLED");

    // Software reset
    uint32_t resetCommand;
    mmio_dword_out(E1000BaseAddress + E1000_REG_CTRL, E1000_REG_CTRL_RESET);
    while( ((resetCommand = mmio_dword_in(E1000BaseAddress + E1000_REG_CTRL)) & E1000_REG_CTRL_RESET) != 0) { kprintf("Reset Comm = %x\n", resetCommand); }

    kprint("E1000 Software Reset done!\n");

    // Enable IRQ callback
    kprintf("RTL IRQ Number: %u\n", _pci_address.irq);

    register_interrupt_handler(_pci_address.irq + 32, E1000_handler);   // our IRQ handlers are shifted by 32 positions (IRQ0=32), may be subject to change?

    return 0;
}

static void i8254x_lock_eeprom() {
    uint32_t tmp = mmio_dword_in(_pci_address.BAR0 + E1000_REG_EECD);

    mmio_dword_out(_pci_address.BAR0 + E1000_REG_EECD, (tmp | E1000_REG_EECD_EE_REQ));
    do {
        tmp = mmio_dword_in(_pci_address.BAR0 + E1000_REG_EECD);
    } while ((tmp & E1000_REG_EECD_EE_GNT) == 0);
}

static void i8254x_unlock_eeprom() {
    uint32_t tmp = mmio_dword_in(_pci_address.BAR0 + E1000_REG_EECD);

    mmio_dword_out(_pci_address.BAR0 + E1000_REG_EECD, (tmp & (~E1000_REG_EECD_EE_REQ)));
}

uint8_t* E1000GetMACAddress() {
    uint8_t* buff = kmalloc(6);

    i8254x_lock_eeprom();
    *((uint16_t *)buff) = i8254x_read_eeprom(0x00);
    *((uint16_t *)(buff + 2)) = i8254x_read_eeprom(0x01);
    *((uint16_t *)(buff + 4)) = i8254x_read_eeprom(0x02);
    i8254x_unlock_eeprom();

    return buff;
}

static void E1000_handler(registers_t reg) {
    kprint("Reached E1000 IRQ handler\n");

    UNUSED(reg);
}
