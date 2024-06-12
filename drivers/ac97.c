#include "ac97.h"

#define REGISTER_BUS_GLOBAL_CONTROL     0x2C
#define REGISTER_BUS_GLOBAL_STATUS      0X30

#define REGISTER_MIXER_RESET            0x00
#define REGISTER_MIXER_MASTER_VOLUME    0X02
#define REGISTER_MIXER_AUX_VOLUME       0x04
#define REGISTER_MIXER_PCM_VOLUME       0x18

uint32_t AC97NAMixerRegister;
uint32_t AC97NABusRegister;

uint32_t identifyAC97() {
    // for some reason, the address may be misaligned (let's only reset the last 4 bits for now)
    uint32_t addr = getDeviceBAR0(0x8086, 0x2415);  // 82801AA AC'97 Audio Controller
    AC97NAMixerRegister = (addr & 0xFFFFFFF0);

    addr = getDeviceBAR1(0x8086, 0x2415);  // 82801AA AC'97 Audio Controller
    AC97NABusRegister = (addr & 0xFFFFFFF0);

    // should probably also check class 0x04 and subclass 0x01
    return AC97NAMixerRegister;
}

uint8_t initializeAC97() {
    kprintf("AC97 Init IO Addr: %u\n", AC97NAMixerRegister);
    // First check if PCI Bus Mastering is enabled
    uint16_t commandReg = pciConfigReadWord(0, 3, 0, 4);
    uint8_t enabled = (uint8_t)(commandReg & 0x5);  // PCI Bus Mastering + I/O

    if (enabled == 0) {
        kprint("AC97 PCI Bus Mastering is DISABLED. Enabling now...\n");
        // we MUST enable PCI Bus Mastering
        uint8_t mask = commandReg |= 5;     // a fancier way of writing 4
        pciConfigWriteWord(0, 3, 0, 4, mask);

        // check value
        commandReg = pciConfigReadWord(0, 3, 0, 4);
        enabled = (uint8_t)(commandReg & 0x5);
    }

    kprintf("PCI Bus Mastering is %s\n", enabled == 5 ? "ENABLED" : "DISABLED");

    // Power on the device
    port_dword_out( AC97NABusRegister + REGISTER_BUS_GLOBAL_CONTROL, 0x2);
    kprint("AC97 Powered On (Without interrupts)\n");

    // Reset registers
    port_word_out(AC97NAMixerRegister + REGISTER_MIXER_RESET, 0x10);    // it can be any value

    // Print card info
    uint32_t status = port_dword_in(AC97NABusRegister + REGISTER_BUS_GLOBAL_STATUS);
    if (status & (1 << 22)) {
        kprint("AC97: 20 bit samples are supported\n");
    } else {
        kprint("AC97: Only 16 bit samples are supported\n");
    }

    uint16_t capabilities = port_word_in(AC97NAMixerRegister + REGISTER_MIXER_RESET);
    if (capabilities & (1 << 4)) {
        kprint("Headphones output detected\n");

        if (port_word_in(AC97NAMixerRegister + REGISTER_MIXER_AUX_VOLUME) == 0x8000) {
            kprint("Currently using HEADPHONE OUTPUT\n");
        }
    } else {
        kprint("NO Headphone output detected\n");
    }

    // Set volume
    port_word_out(AC97NAMixerRegister + REGISTER_MIXER_AUX_VOLUME, 0);
    port_word_out(AC97NAMixerRegister + REGISTER_MIXER_MASTER_VOLUME, 0);
    port_word_out(AC97NAMixerRegister + REGISTER_MIXER_PCM_VOLUME, 0);
}
