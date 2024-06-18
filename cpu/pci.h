#ifndef _PCI_H
#define _PCI_H

#include <stdint.h>

struct PCIAddressInfo {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint32_t BAR0;
    uint32_t BAR1;
    uint8_t irq;
};

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pciConfigWriteWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t word);
uint16_t pciCheckVendor(uint8_t bus, uint8_t slot);
void checkDevice(uint8_t bus, uint8_t device);
void checkFunction(uint8_t bus, uint8_t device, uint8_t function);
void checkBus(uint8_t bus);
void checkAllBuses(void);
void printHeaderBytes(uint8_t bus, uint8_t device, uint8_t function);

uint32_t getDeviceBAR0(uint16_t vendorID, uint16_t deviceID);
uint32_t getDeviceBAR1(uint16_t vendorID, uint16_t deviceID);
uint8_t getIRQNumber(uint16_t vendorID, uint16_t deviceID);
void getDeviceInfo(uint16_t vendorID, uint16_t deviceID, struct PCIAddressInfo* pci);

#endif
