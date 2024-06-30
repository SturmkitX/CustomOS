#ifndef _MIDI_H
#define _MIDI_H

#include <stdint.h>

void play_midi(void* stream, uint32_t len);
void play_midi_file(char* filename);
void init_midi(char* sfName, float sample_rate);

#endif
