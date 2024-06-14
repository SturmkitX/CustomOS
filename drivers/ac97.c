#include "ac97.h"

#define REGISTER_BUS_GLOBAL_CONTROL     0x2C
#define REGISTER_BUS_GLOBAL_STATUS      0X30
#define REGISTER_BUS_PCM_OUT            0X10

#define REGISTER_BUS_TRANSFER_CONTROL_OFFSET    0X0B
#define REGISTER_BUS_CONTROL_STATUS_OFFSET      0X06
#define REGISTER_BUS_BDL_ADDRESS_OFFSET         0X00
#define REGISTER_BUS_BDL_NO_ENTRIES_OFFSET      0X05
#define REGISTER_BUS_BDL_CURR_ENTRY_OFFSET      0X04

#define REGISTER_MIXER_RESET            0x00
#define REGISTER_MIXER_MASTER_VOLUME    0X02
#define REGISTER_MIXER_AUX_VOLUME       0x04
#define REGISTER_MIXER_PCM_VOLUME       0x18
#define REGISTER_MIXER_PCM_FRONT_SAMPLE_RATE    0X2C

#define MIN(a, b) (a < b ? a : b)

uint32_t AC97NAMixerRegister;
uint32_t AC97NABusRegister;

static uint8_t _audio_sample_size = 2;  // 16 bits

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

    if (enabled != 5) {
        kprint("AC97 PCI Bus Mastering is DISABLED. Enabling now...\n");
        // we MUST enable PCI Bus Mastering
        uint8_t mask = commandReg |= 5;     // a fancier way of writing 4
        pciConfigWriteWord(0, 3, 0, 4, mask);

        // check value
        commandReg = pciConfigReadWord(0, 3, 0, 4);
        enabled = (uint8_t)(commandReg & 0x5);
    }

    kprintf("PCI Bus Mastering is %s (%u)\n", enabled == 5 ? "ENABLED" : "DISABLED", enabled);

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

    uint32_t ctrl = (port_dword_in( AC97NABusRegister + REGISTER_BUS_GLOBAL_CONTROL) >> 20);
    kprintf("PCM Output and Channels: %x\n", ctrl);

    // Set volume
    port_word_out(AC97NAMixerRegister + REGISTER_MIXER_AUX_VOLUME, 0);
    port_word_out(AC97NAMixerRegister + REGISTER_MIXER_MASTER_VOLUME, 0);
    port_word_out(AC97NAMixerRegister + REGISTER_MIXER_PCM_VOLUME, 0);

    return 1;
}

void playAudio(uintptr_t buffer, uint32_t bufferLength) {
    struct AC97BufferDescriptor* desc = (struct AC97BufferDescriptor*) kmalloc(sizeof(struct AC97BufferDescriptor) * AC97_BDL_NO_ENTRIES);
    uint32_t offset = 0, inc;
    uint32_t i;

    // must add checks
    for (i=0; i < AC97_BDL_NO_ENTRIES && offset < bufferLength; i++) {
        desc[i].data = buffer + offset;

        inc = MIN((bufferLength - offset) / _audio_sample_size, AC97_BDL_NO_SAMPLES_MAX);
        desc[i].no_samples = inc;
        desc[i].control = inc < AC97_BDL_NO_SAMPLES_MAX ? (1 << 14) : 0;    // send Last Entry bit if necessary

        offset += inc * _audio_sample_size;
    }

    // Set reset bit of output channel and wait for clear
    port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET, (1 << 1));
    while (port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET) != 0) {}

    kprint("Successfully reset PCM Output channel\n");

    // Write BDL address
    port_dword_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_ADDRESS_OFFSET, (uintptr_t)desc);

    // Write number of entries (aka Last Valid Entry)
    port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_NO_ENTRIES_OFFSET, 31);   // only 5 bits are used (0..31)

    // Start DMA transfer
    port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET, (1 << 0));

    uint16_t sample_rate = port_word_in(AC97NAMixerRegister + REGISTER_MIXER_PCM_FRONT_SAMPLE_RATE);
    kprintf("PCM Front Sample Rate: %u\n", sample_rate);

    // Wait for play to finish
    uint16_t status = 0;
    uint8_t curr_entry = 0;
    uint8_t last_entry = 0;     // probably should be initialized with something
    do {
        status = port_word_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_CONTROL_STATUS_OFFSET);
        curr_entry = port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_CURR_ENTRY_OFFSET);
        kprintf("%u %u\n", status, curr_entry);

        if (curr_entry != last_entry) {
            desc[last_entry].data = buffer + offset;

            inc = MIN((bufferLength - offset) / _audio_sample_size, AC97_BDL_NO_SAMPLES_MAX);
            desc[last_entry].no_samples = inc;
            desc[last_entry].control = inc < AC97_BDL_NO_SAMPLES_MAX ? (1 << 14) : 0;    // send Last Entry bit if necessary

            offset += inc * _audio_sample_size;

            port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_NO_ENTRIES_OFFSET, last_entry);   // only 5 bits are used (0..31)
            last_entry = curr_entry;
        }
    } while ((status & 2) == 0);

    kprint("Music transmission ended\n");
}
