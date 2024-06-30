#include "midi.h"
#include "../libc/mem.h"
#include "../cpu/timer.h"
#include "../drivers/vfs.h"
#include "../drivers/ac97.h"

#define TML_MALLOC  kmalloc
#define TML_REALLOC krealloc
#define TML_FREE    kfree
#define TML_MEMCPY  memory_copy
#define TML_NO_STDIO

#define TSF_MALLOC  kmalloc
#define TSF_REALLOC krealloc
#define TSF_FREE    kfree
#define TSF_MEMCPY  memory_copy
#define TSF_MEMSET  memory_set
#define TSF_NO_STDIO

#define TML_IMPLEMENTATION
#include "../tinymidipcm/tml.h"

#define TSF_IMPLEMENTATION
#include "../tinymidipcm/tsf.h"

#include <stdint.h>

// Holds the global instance pointer
static tsf* g_TinySoundFont;

// Holds global MIDI playback state
static double g_Msec;               //current playback time
static tml_message* g_MidiMessage;  //next message to be played

static float g_SampleRate = 48000.0f;

static tml_message* MidiPlayCallback(void* data, char* stream, int len)
{
	tml_message* g_MidiMessage = (tml_message*)data;

	//Number of samples to process
	int SampleBlock, SampleCount = (len / (2 * sizeof(short))); //2 output channels
	for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, stream += (SampleBlock * (2 * sizeof(short))))
	{
		//We progress the MIDI playback and then process TSF_RENDER_EFFECTSAMPLEBLOCK samples at once
		if (SampleBlock > SampleCount) SampleBlock = SampleCount;

		//Loop through all MIDI messages which need to be played up until the current playback time
		for (g_Msec += SampleBlock * (1000.0 / g_SampleRate); g_MidiMessage && g_Msec >= g_MidiMessage->time; g_MidiMessage = g_MidiMessage->next)
		{
			switch (g_MidiMessage->type)
			{
			case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
				tsf_channel_set_presetnumber(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->program, (g_MidiMessage->channel == 9));
				break;
			case TML_NOTE_ON: //play a note
				tsf_channel_note_on(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key, g_MidiMessage->velocity / 127.0f);
				break;
			case TML_NOTE_OFF: //stop a note
				tsf_channel_note_off(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key);
				break;
			case TML_PITCH_BEND: //pitch wheel modification
				tsf_channel_set_pitchwheel(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->pitch_bend);
				break;
			case TML_CONTROL_CHANGE: //MIDI controller messages
				tsf_channel_midi_control(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->control, g_MidiMessage->control_value);
				break;
			}
		}

		// Render the block of audio samples in float format
		tsf_render_short(g_TinySoundFont, (short*)stream, SampleBlock, 0);
	}

	return g_MidiMessage;
}

void init_midi(char* sfName, float sample_rate) {
	// Define the desired audio output format we request
	/*SDL_AudioSpec OutputAudioSpec;
	OutputAudioSpec.freq = 11025;
	OutputAudioSpec.format = AUDIO_S16;
	OutputAudioSpec.channels = 2;
	OutputAudioSpec.samples = 4096;
	OutputAudioSpec.callback = AudioCallback;*/

	// Load the SoundFont from a file
    struct VFSEntry *tsf = vfs_open(sfName, "rb");
    if (!tsf) {
        kprintf("%s could not be found\n", sfName);
        return;
    }
    uint8_t *sffile = kmalloc(tsf->size_bytes);
    vfs_read(tsf, sffile, tsf->size_bytes);

	g_TinySoundFont = tsf_load_memory(sffile, tsf->size_bytes);
	if (!g_TinySoundFont)
	{
		kprint("Could not load SoundFont\n");
		return 1;
	}

	//Initialize preset on special 10th MIDI channel to use percussion sound bank (128) if available
	tsf_channel_set_bank_preset(g_TinySoundFont, 9, 128, 0);

	// Set the SoundFont rendering output mode
	tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, sample_rate, 0.0f);

	g_SampleRate = sample_rate;
	g_Msec = 0;
}

void play_midi(void* data, uint32_t size) {
	tml_message* TinyMidiLoader = tml_load_memory((char*)data, size);

	uint32_t secsize = 10 * 11502 * 4;	// 2 channels, 2 bytes per sample
	char* pcm = (char*) kmalloc(secsize);	// synthesize up to 1 second

	double msec_old = g_Msec;

	TinyMidiLoader = MidiPlayCallback(TinyMidiLoader, pcm, secsize);
	char* newpcm = pcm;
	uint32_t currsize = secsize;
	
	while ((newpcm = playAudio(newpcm, currsize))) {
		playAudio_Callback();
		kprintf("Waiting for buffer to empty %u\n", newpcm);
		currsize = secsize - (uint32_t)(newpcm - pcm);
	}

	while(playAudio_Callback()) {}
}

void play_midi_file(char* filename) {
	struct VFSEntry *tml = vfs_open(filename, "rb");
	if (!tml) {
		kprintf("%s could not be found\n", filename);
		return;
	}
	uint8_t *midifile = kmalloc(tml->size_bytes);
	vfs_read(tml, midifile, tml->size_bytes);

	play_midi(midifile, tml->size_bytes);
}

