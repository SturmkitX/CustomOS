#include "vfs.h"
#include "../drivers/ata_pio_drv.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/function.h"
#include "../drivers/screen.h"

#define VFS_ENTRIES_MAX_NUM     1024
#define FILE_OPS_MEM            0x04000000              // starts at 64 MB
static struct VFSEntry headers[VFS_ENTRIES_MAX_NUM];   // maximum 1024 files for now

// static char openfilename[16];   // max num of characters
static uint8_t isBuffered = 0;

struct VFSEntry* vfs_open(const char* filename, const char* mode)
{
    uint32_t i;

    // read file table
    kprintf("Sizeof headers = %u\n", sizeof(headers));
    ata_pio_read48(VFS_START_SECTOR, sizeof(headers) / 512, headers);

    for (i=0; i < VFS_ENTRIES_MAX_NUM && headers[i].name[0] != 0; i++) {
        if (strcmp(headers[i].name, filename) == 0) {
            kprintf("Found file %s\n", filename);

            headers[i].current = 0;
            if (mode[0] == 'w' && strchr(mode, 'a') < 0) {
                headers[i].size_bytes = 0;
                headers[i].size_sectors = 0;
            }

            isBuffered = 0;
            return &headers[i];
        }
    }

    if (i < VFS_ENTRIES_MAX_NUM && mode[0] == 'w') {
        // Hit an empty space
        headers[i].current = 0;
        headers[i].size_bytes = 0;
        headers[i].size_sectors = 0;
        headers[i].start_sector = VFS_START_SECTOR + sizeof(headers) / 512 + i * 16000;      // each file can have up to 8 MB of data (originally was 4 MB, but it was not enough to accommodate doom1.wad)

        uint16_t len = MIN(strlen(filename), 16);
        memory_copy(headers[i].name, filename, len);

        // let's flush the contents immediately
        // ata_pio_write48(headers[i].start_sector, headers[i].size_sectors, tmp);

        uint32_t sector = i * sizeof(struct VFSEntry) / 512;
        ata_pio_write48(VFS_START_SECTOR + sector, 1, (uintptr_t)headers + sector * 512);

        isBuffered = 0;
        return &headers[i];
    }

    return NULL;
}

void vfs_close(struct VFSEntry* handle)
{
    // let us flush the data
    // ata_pio_write48(handle->start_sector, handle->size_sectors,)
    handle->current = 0;
}

int vfs_read(struct VFSEntry* handle, void *buf, int count)
{
    // For now, just read the entire file (later we will skip all unneeded sectors)
    // I don't want to use too many kmallocs, as memory is not properly freed yet
    uint8_t* tmp = (uint8_t*) FILE_OPS_MEM;
    uint32_t toread = MIN(count, handle->size_bytes - handle->current);

    if (toread == 0)
        return 0;

    if (isBuffered == 0) {
        ata_pio_read48(handle->start_sector, handle->size_sectors, tmp);
        isBuffered = 1;
        // memory_copy(openfilename, handle->name, 16);
    }
    
    memory_copy(buf, tmp + handle->current, toread);

    handle->current += toread;

    return toread;
}

int vfs_write(struct VFSEntry* handle, const void *buf, int count)
{
    uint8_t* tmp = (uint8_t*) FILE_OPS_MEM;

    if (handle->size_bytes > 0) ata_pio_read48(handle->start_sector, handle->size_sectors, tmp);
    memory_copy(tmp + handle->size_bytes, buf, count);

    handle->size_bytes += count;
    handle->size_sectors = handle->size_bytes / 512 + (handle->size_bytes % 512 > 0);

    // let's flush the contents immediately
    ata_pio_write48(handle->start_sector, handle->size_sectors, tmp);
    kprintf("Write data: Sector = %u (%x), size = %u\n", handle->start_sector, handle->start_sector, handle->size_sectors);

    // in order not to disrupt alignment (for now), I will compute the index
    uint32_t index = (handle->start_sector - VFS_START_SECTOR - sizeof(headers) / 512) / 16000;
    uint32_t sector = index * sizeof(struct VFSEntry) / 512;
    kprintf("Write index = %u, sector = %u\n", index, sector);
    kprintf("Values to write: %u %u %u %u\n", handle->start_sector, handle->size_bytes, handle->size_sectors, handle->current);
    ata_pio_write48(VFS_START_SECTOR + sector, 1, (uintptr_t)headers + sector * 512);
    kprintf("Write header: Sector = %u (%x)\n", VFS_START_SECTOR, VFS_START_SECTOR);

    return count;
}

int vfs_seek(struct VFSEntry* handle, int offset, seek_t origin)
{
    switch (origin) {
        case SEEK_CUR:
            if ((int)(handle->current) + offset < 0 || handle->current + offset > handle->size_bytes) {
                return 1;
            }
            handle->current += offset;
            break;
        case SEEK_SET:
            if (offset < 0 || offset > handle->size_bytes) {
                return 1;
            }
            handle->current = offset;
            break;
        default:
            if (offset > 0 || -offset > handle->size_bytes) {
                return 1;
            }
            handle->current = handle->size_bytes + offset;
    }

    return 0;
}

int vfs_tell(struct VFSEntry* handle)
{
    return (int)handle->current;
}

int vfs_eof(struct VFSEntry* handle)
{
    return (handle->current == handle->size_bytes);
}
