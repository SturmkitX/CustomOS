#include "ac97.h"

#include "../cpu/pci.h"

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
#define REGISTER_MIXER_EXT_CAP          0X28
#define REGISTER_MIXER_CTL_EXT_CAP      0X2A
#define REGISTER_MIXER_PCM_FRONT_SAMPLE_RATE    0X2C

#define MIN(a, b) (a < b ? a : b)

uint32_t AC97NAMixerRegister;
uint32_t AC97NABusRegister;

static uint8_t _audio_sample_size = 2;  // 16 bits

static struct PCIAddressInfo _pci_address;

static uint32_t AC97AddedEntries = 0;       // should hold for some time, depends on frequency
static uint32_t AC97ProcessedEntries = 0;
static uint8_t AC97LastEntry = 255;
static struct AC97BufferDescriptor AC97BDLDesc[AC97_BDL_NO_ENTRIES];

uint8_t identifyAC97() {
    // for some reason, the address may be misaligned (let's only reset the last 4 bits for now)
    getDeviceInfo(0x8086, 0x2415, &_pci_address);  // 82801AA AC'97 Audio Controller
    AC97NAMixerRegister = (_pci_address.BAR0 & 0xFFFFFFF0);
    AC97NABusRegister = (_pci_address.BAR1 & 0xFFFFFFF0);

    // should probably also check class 0x04 and subclass 0x01
    return (_pci_address.vendor_id != 0xFFFF && _pci_address.device_id != 0xFFFF);
}

uint8_t initializeAC97(uint16_t sample_rate) {
    kprintf("AC97 Init IO Addr: %u\n", AC97NAMixerRegister);
    // First check if PCI Bus Mastering is enabled
    uint16_t commandReg = pciConfigReadWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4);
    uint8_t enabled = (uint8_t)(commandReg & 0x5);  // PCI Bus Mastering + I/O

    if (enabled != 5) {
        kprint("AC97 PCI Bus Mastering is DISABLED. Enabling now...\n");
        // we MUST enable PCI Bus Mastering
        uint8_t mask = commandReg |= 5;     // a fancier way of writing 4
        pciConfigWriteWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4, mask);

        // check value
        commandReg = pciConfigReadWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4);
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

    // Change sample rate if needed
    if (sample_rate != DEFAULT_SAMPLE_RATE) {
        // Check if we can do it
        if ((port_word_in(AC97NAMixerRegister + REGISTER_MIXER_EXT_CAP) & 1) == 1) {
            kprintf("Changing sample rate to %u\n", sample_rate);
            uint16_t ctlext = port_word_in(AC97NAMixerRegister + REGISTER_MIXER_CTL_EXT_CAP);
            port_word_out(AC97NAMixerRegister + REGISTER_MIXER_CTL_EXT_CAP, (ctlext | 1));

            port_word_out(AC97NAMixerRegister + REGISTER_MIXER_PCM_FRONT_SAMPLE_RATE, sample_rate);
        }
    }

    stopAudio();

    return 1;
}

void resumeAudio() {
    uint8_t stat = port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET);
    stat |= 1;
    port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET, stat);
}

void pauseAudio() {
    uint8_t stat = port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET);
    stat &= ~1;
    port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET, stat);

    kprint("Audio paused\n");
}

uint8_t isAudioPlaying() {
    uint8_t stat = port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET);

    return ((stat & 1) > 0);
}

void stopAudio() {
    // feels like it is not properly reset
    pauseAudio();

    // Set reset bit of output channel and wait for clear
    port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET, (1 << 1));
    while (port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_TRANSFER_CONTROL_OFFSET) != 0) {}

    // // Write BDL address
    // port_dword_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_ADDRESS_OFFSET, (uintptr_t)AC97BDLDesc);

    AC97AddedEntries = 0;
    AC97ProcessedEntries = 0;

    kprint("Successfully reset PCM Output channel\n");
}

uint8_t playAudio_Callback() {
    // Wait for play to finish
    
    uint8_t curr_entry = port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_CURR_ENTRY_OFFSET);
    if (AC97LastEntry == 255) {
        AC97LastEntry = curr_entry;
    }

    if (AC97LastEntry != curr_entry) {
        AC97LastEntry = curr_entry;
        AC97ProcessedEntries++;
        kprintf("%u\n", curr_entry);
    }

    uint16_t status = port_word_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_CONTROL_STATUS_OFFSET);
    uint8_t isEnded = ((status & 2) > 0);

    if (isEnded) {
        stopAudio();
        kprint("Music transmission ended\n");
    }
        
    curr_entry = port_byte_in(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_CURR_ENTRY_OFFSET);

    // AC97ProcessedEntries++;     // needs to be done when song is finished
    kprintf("%u %u\n", curr_entry, isEnded);

    return (!isEnded);
}

uintptr_t playAudio(uintptr_t buffer, uint32_t bufferLength) {
    uint32_t offset = 0, inc;
    uint32_t i;
    uint8_t pos;

    if (!isAudioPlaying()) {
        AC97AddedEntries = 0;
        AC97ProcessedEntries = 0;

        // must add checks
        // may overwrite existing songs if audio is too large
        for (i=0; i < AC97_BDL_NO_ENTRIES_BUFFER && offset < bufferLength; i++) {
            pos = (AC97AddedEntries % AC97_BDL_NO_ENTRIES);
            AC97AddedEntries++;
            kprintf("Added BDL entries: %u\n", AC97AddedEntries);

            AC97BDLDesc[i].data = buffer + offset;

            inc = MIN((bufferLength - offset) / _audio_sample_size, AC97_BDL_NO_SAMPLES_MAX);
            AC97BDLDesc[i].no_samples = inc;

            offset += inc * _audio_sample_size;
            AC97BDLDesc[i].control = 0;    // send Last Entry bit if necessary (bit 14)
        }

        // Write BDL address
        port_dword_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_ADDRESS_OFFSET, (uintptr_t)AC97BDLDesc);
        

        // Write number of entries (aka Last Valid Entry)
        port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_NO_ENTRIES_OFFSET, i-1);   // only 5 bits are used (0..31)

        resumeAudio();
    } else {
        uint8_t available_entries = AC97_BDL_NO_ENTRIES_BUFFER - (AC97AddedEntries - AC97ProcessedEntries);
        for (i=0; i < available_entries && offset < bufferLength; i++) {
            pos = (AC97AddedEntries % AC97_BDL_NO_ENTRIES);
            AC97AddedEntries++;

            AC97BDLDesc[pos].data = buffer + offset;

            inc = MIN((bufferLength - offset) / _audio_sample_size, AC97_BDL_NO_SAMPLES_MAX);
            AC97BDLDesc[pos].no_samples = inc;

            offset += inc * _audio_sample_size;
            AC97BDLDesc[pos].control = 0;    // send Last Entry bit if necessary
        }

        // Write number of entries (aka Last Valid Entry)
        port_byte_out(AC97NABusRegister + REGISTER_BUS_PCM_OUT + REGISTER_BUS_BDL_NO_ENTRIES_OFFSET, pos);   // only 5 bits are used (0..31)
    }

    // return NULL is nothing is left to play, otherwise return location of current position
    return (offset == bufferLength) ? 0 : (buffer + offset);

    // uint16_t sample_rate = port_word_in(AC97NAMixerRegister + REGISTER_MIXER_PCM_FRONT_SAMPLE_RATE);
    // kprintf("PCM Front Sample Rate: %u\n", sample_rate);
}

uint8_t isAudioBufferFull() {
    return (AC97AddedEntries - AC97ProcessedEntries >= AC97_BDL_NO_ENTRIES_BUFFER);
}
