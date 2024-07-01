#include "doom.h"

#include "../PureDOOM/src/DOOM/DOOM.h"

#include "../drivers/vfs.h"
#include "../drivers/screen.h"
#include "../drivers/serial.h"
#include "../drivers/ac97.h"
#include "../libc/function.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../cpu/timer.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"

#include "midi.h"

// don't know exactly what ENV values are needed
char* waddir = "WADS";
char *homedir = ".";

static void* custom_open_impl(const char* filename, const char* mode)
{
    return vfs_open(filename, mode);
}

static void custom_close_impl(void* handle)
{
    vfs_close(handle);
}

static int custom_read_impl(void* handle, void *buf, int count)
{
    // write_string_serial("DOOM Reading from HDD bytes");
    // write_string_serial(doom_itoa(count, 10));
    // write_string_serial("\r\n");
    return vfs_read(handle, buf, count);
}

static int custom_write_impl(void* handle, const void *buf, int count)
{
    write_string_serial("Doom is writing to file\n");
    return vfs_write(handle, buf, count);
}

static int custom_seek_impl(void* handle, int offset, doom_seek_t origin)
{
    return vfs_seek(handle, offset, origin);
}

static int custom_tell_impl(void* handle)
{
    return vfs_tell(handle);
}

static int custom_eof_impl(void* handle)
{
    return vfs_eof(handle);
}

static void custom_print_impl(const char* str)
{
    write_string_serial(str);
}

static void* custom_malloc_impl(int size)
{
    uintptr_t tmp = kmalloc((size_t)size);
    // char msg[50];

    // write_string_serial("DOOM Allocated memory at ");
    // write_string_serial(doom_itoa(tmp, 10));
    // write_string_serial(" with size ");
    // write_string_serial(doom_itoa(size, 10));
    // write_string_serial("\r\n");
    return tmp;   // its current implementation does not really free memory, so app may crash in a couple of seconds due to insufficient memory
}

static void custom_free_impl(void* ptr)
{
    kfree(ptr);
}

static void custom_gettime_impl(int* sec, int* usec)
{
    uint32_t ticks = get_ticks();   // each tick is 1 ms (0.001 s, 1000 us)

    *sec = ticks / 1000 + 1719346785;
    *usec = (ticks % 1000) * 1000;
}

static void custom_exit_impl(int code)
{
    kprintf("Exiting DOOM with code %u\n", code);
    asm("hlt");

    // in case HLT does not work
    while (1) {}
}

static char* custom_getenv_impl(const char* var)
{
    kprintf("Looking for ENV variable %s...\n", var);
    if (strcmp(var, "DOOMWADDIR") == 0) {
        return waddir;
    }

    return homedir;
}

void initialize_doom() {
    init_serial();
    doom_set_print(custom_print_impl);
    doom_set_malloc(custom_malloc_impl, custom_free_impl);
    doom_set_file_io(custom_open_impl,
                        custom_close_impl,
                        custom_read_impl,
                        custom_write_impl,
                        custom_seek_impl,
                        custom_tell_impl,
                        custom_eof_impl);
    doom_set_gettime(custom_gettime_impl);
    doom_set_exit(custom_exit_impl);
    doom_set_getenv(custom_getenv_impl);

    // Change default bindings to modern mapping
    doom_set_default_int("key_up",          DOOM_KEY_W);
    doom_set_default_int("key_down",        DOOM_KEY_S);
    doom_set_default_int("key_left",        DOOM_KEY_A);
    doom_set_default_int("key_right",       DOOM_KEY_D);
    doom_set_default_int("key_use",         DOOM_KEY_E);
    doom_set_default_int("key_fire",         DOOM_KEY_R);
    doom_set_default_int("mouse_move",      0); // Mouse will not move forward

    char* argv[] = {"C:\\doom\\doom.exe"};
    doom_init(1, argv, 0);

    set_scale_factor(2);
    uint32_t resx = getResWidth();
    uint32_t resy = getResHeight();
    kprintf("Resolution after Scale by 2: %ux%u\n", getResWidth(), getResHeight());

    uint8_t key, released, special;
    while(poll_key(&key, &released, &special)) {}
    // initializeAC97(11025);

    uint32_t start_tick = get_ticks();
    uint32_t curr_tick = start_tick;
    const uint32_t ticks_to_wait = 1000 / 60;   // 60 fps
    const uint32_t ticks_sfx = 1000 / 20;
    const uint32_t ticks_music = 1000 / 140;
    uint32_t start_tick_music = start_tick;
    uint32_t start_tick_sfx = start_tick;

    uint16_t* music_buffer = (uint16_t*) kmalloc(11025 * 4);    // store 1 second on sfx
    uint32_t music_buffind = 0;

    uint8_t* sfx_buffer = (uint8_t*) kmalloc(2048 * 20); // store 20 chunks
    uint8_t sfx_buffind = 0;
    uint8_t last_scancode = 255, last_released = 0;

    // init_midi("scc1t2.sf2", 11025.0);

    while (1) {
        doom_update();
        kprint("Doom updated\n");

        if (poll_key(&key, &released, &special)) {
            if (key != last_scancode || last_released != released) {
                char doomkey = key_to_ascii(key, special);
                if (released)
                    doom_key_up(doomkey);
                else
                    doom_key_down(doomkey);

                last_scancode = key;
                last_released = released;
            }
        }

        // assumes tick at 1ms (but will change soon)
        curr_tick = get_ticks();
        if (curr_tick - start_tick >= ticks_to_wait) {
            uint8_t* fb = doom_get_framebuffer(3);

            // needs improvement
            draw_icon(0, 0, resx, resy, fb);

            start_tick = curr_tick;
        }
        

        if (curr_tick - start_tick_sfx >= ticks_sfx) {
            int16_t* buffer = doom_get_sound_buffer(2048);
            memory_copy(sfx_buffer + sfx_buffind * 2048, buffer, 2048);
            sfx_buffind++;

            if (sfx_buffind % 10 == 0) {
                playAudio(sfx_buffer + (sfx_buffind - 10) * 2048, 2048 * 10);
                sfx_buffind %= 20;;
            }

            start_tick_sfx = curr_tick;
        }

        // if (curr_tick - start_tick_music >= ticks_music) {
        //     // int16_t* buffer = doom_get_sound_buffer(2048);
        //     uint32_t midi_msg;

        //     while (midi_msg = doom_tick_midi()) {
        //         play_midi(midi_msg, 4096);  // up to 4KB
        //     }
        //     // playAudio(buffer, 2048);

        //     start_tick_music = curr_tick;
        // }

        uint32_t midi_msg = doom_tick_midi();
        if (midi_msg) {
            music_buffer[music_buffind++] = (uint16_t)(midi_msg >> 8);
            if (music_buffind % 10000 == 0) {
                play_midi(music_buffer + (music_buffind - 10000), 22050);
                music_buffind %= 20000;
            }
        }

        playAudio_Callback();
    }

}
