#ifndef _AC97_H
#define _AC97_H

#include <stdint.h>
#include <stddef.h>

#define AC97_BDL_NO_ENTRIES     32
#define AC97_BDL_NO_SAMPLES_MAX 0xFFFE

#define DEFAULT_SAMPLE_RATE     48000

struct AC97BufferDescriptor {
    uintptr_t data;
    uint16_t no_samples;
    uint16_t control;
};

uint8_t identifyAC97();
uint8_t initializeAC97(uint16_t sample_rate);
void playAudio(uintptr_t buffer, uint32_t bufferLength);

#endif
