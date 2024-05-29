#include "pci.h"
#include "../drivers/screen.h"
#include "ports.h"

// offset refers to the byte offset (0-3) inside the dword
uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
 
    // Write out the address
    port_dword_out(0xCF8, address);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    tmp = (uint16_t)((port_dword_in(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

void pciConfigWriteWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t word) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
 
    port_dword_out(0xCF8, address);
    port_dword_out(0xCFC, word);
}

uint8_t getHeaderType(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t data = pciConfigReadWord(bus, slot, func, 0x0E);
    kprintf("Read Header type: %x\n", data);
    return (uint8_t)(data & 0xFF);
}

uint16_t getVendorID(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t data = pciConfigReadWord(bus, slot, func, 0x4);
    return data;
}

uint16_t pciCheckVendor(uint8_t bus, uint8_t slot) {
    uint16_t vendor, device;
    /* Try and read the first configuration register. Since there are no
     * vendors that == 0xFFFF, it must be a non-existent device. */
    if ((vendor = pciConfigReadWord(bus, slot, 0, 0)) != 0xFFFF) {
       device = pciConfigReadWord(bus, slot, 0, 2);
       kprintf("Vendor = %x, Device = %x\n", vendor, device);
    } return (vendor);
}

void checkDevice(uint8_t bus, uint8_t device) {
    uint8_t function = 0;

    uint16_t vendorID = getVendorID(bus, device, function);

    // if (vendorID != 0x1111)
        kprintf("Found VendorID %x\n", vendorID);
    if (vendorID == 0xFFFF) return; // Device doesn't exist
    checkFunction(bus, device, function);
    uint8_t headerType = getHeaderType(bus, device, function);
    if( (headerType & 0x80) != 0) {
        // It's a multi-function device, so check remaining functions
        for (function = 1; function < 8; function++) {
            if (getVendorID(bus, device, function) != 0xFFFF) {
                checkFunction(bus, device, function);
            }
        }
    }
}

void checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
}
 
void checkAllBuses(void) {
    uint16_t bus;
    uint8_t device;

    for (bus = 0; bus < 256; bus++) {
        for (device = 0; device < 32; device++) {
            checkDevice(bus, device);
        }
    }
}

void printHeaderBytes(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)device;
    uint32_t lfunc = (uint32_t)function;
    uint32_t tmp;
    uint32_t offset;
 
    for (offset=0; offset < 4; offset += 4) {
        // Create configuration address as per Figure 1
        address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
                // Write out the address
        port_dword_out(0xCF8, address);
        tmp = port_dword_in(0xCFC);
        kprintf("Test Offset %d: %x\n", offset, tmp);
    }
}

uint32_t getDeviceBAR0(uint16_t vendorID, uint16_t deviceID) {
    uint32_t bus, device, address, tmp;
    const uint32_t nodeID = (((uint32_t)deviceID << 16) | vendorID);
    const uint32_t offsetBAR = (0x10 & 0xFC);
    for (bus=0; bus < 256; bus++) {
        for (device=0; device < 32; device++) {
            // Create configuration address as per Figure 1
            address = (uint32_t)((bus << 16) | (device << 11) | ((uint32_t)0x80000000));
                    // Write out the address
            port_dword_out(0xCF8, address);
            tmp = port_dword_in(0xCFC);

            if (tmp == nodeID) {
                address |= offsetBAR;
                port_dword_out(0xCF8, address);
                return port_dword_in(0xCFC);
            }
        }
    }

    return 0xFFFFFFFF;
}

uint8_t getIRQNumber(uint16_t vendorID, uint16_t deviceID) {
    uint32_t bus, device, address, tmp;
    const uint32_t nodeID = (((uint32_t)deviceID << 16) | vendorID);
    const uint32_t offsetIRQ = 0x3C;
    for (bus=0; bus < 256; bus++) {
        for (device=0; device < 32; device++) {
            // Create configuration address as per Figure 1
            address = (uint32_t)((bus << 16) | (device << 11) | ((uint32_t)0x80000000));
                    // Write out the address
            port_dword_out(0xCF8, address);
            tmp = port_dword_in(0xCFC);

            if (tmp == nodeID) {
                address |= offsetIRQ;
                port_dword_out(0xCF8, address);
                tmp = port_dword_in(0xCFC);
                return (uint8_t)(tmp & 0xFF);
            }
        }
    }

    return 0xFF;
}
