#define custom_IMPLEMENTATION 
#include "../PureDOOM/PureDOOM.h"

#include "../drivers/vfs.h"
#include "../libc/function.h"

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