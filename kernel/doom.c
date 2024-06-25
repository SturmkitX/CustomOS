#include "doom.h"

#define DOOM_IMPLEMENTATION 
#include "../PureDOOM/PureDOOM.h"

#include "../drivers/vfs.h"
#include "../drivers/screen.h"
#include "../drivers/serial.h"
#include "../libc/function.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../cpu/timer.h"

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
    // kprintf("DOOM Reading %u bytes from HDD\n", count);
    return vfs_read(handle, buf, count);
}

static int custom_write_impl(void* handle, const void *buf, int count)
{
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
    kprint(str);
}

static void* custom_malloc_impl(int size)
{
    uintptr_t tmp = kmalloc((size_t)size);
    return tmp;   // its current implementation does not really free memory, so app may crash in a couple of seconds due to insufficient memory
}

static void custom_free_impl(void* ptr)
{
    kfree(ptr);
}

static void custom_gettime_impl(int* sec, int* usec)
{
    uint32_t ticks = get_ticks();   // each tick is 1 ms (0.001 s, 1000 us)

    *sec = ticks / 1000;
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

    char** argv = {{"C:\\doom\\doom.exe"}};
    doom_init(1, argv, 0);

    // uint32_t i;

    // kprintf("Waiting for like 5 seconds\n");
    // uint32_t els = 1000 / 35;   // time between frames
    // uint32_t totalframes = 35 * 5;

    // while (totalframes > 0) {
    //     doom_update();
    //     sleep(els);
    //     totalframes--;
    // }

    // kprintf("Updated DOOM for 5 seconds\n");

    // uint8_t* fb = doom_get_framebuffer(1);
    // for (i=0; i < 320; i++) {
    //     kprintf("%u ", fb[i]);
    // }
}
