#ifndef _AC97_H
#define _AC97_H

#include <stdint.h>
#include <stddef.h>

#define AC97_BDL_NO_ENTRIES     32
#define AC97_BDL_NO_SAMPLES_MAX 0xFFFE

struct AC97BufferDescriptor {
    uintptr_t data;
    uint16_t no_samples;
    uint16_t control;
};

uint32_t identifyAC97();
uint8_t initializeAC97();
void playAudio(uintptr_t buffer, uint32_t bufferLength);

#endif
