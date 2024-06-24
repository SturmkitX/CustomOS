// Virtual File System, but our own shitty variant
#ifndef _VFS_H
#define _VFS_H

#include <stdint.h>

#define VFS_START_SECTOR 10000

typedef enum
{
    SEEK_CUR = 1,
    SEEK_END = 2,
    SEEK_SET = 0
} seek_t;

struct VFSEntry {
    uint32_t start_sector;
    uint32_t size_bytes;
    uint32_t size_sectors;
    uint32_t current;   // used for reads
    char name[16];      // max 15 characters for now
};

struct VFSEntry* vfs_open(const char* filename, const char* mode);
void vfs_close(struct VFSEntry* handle);
int vfs_read(struct VFSEntry* handle, void *buf, int count);
int vfs_write(struct VFSEntry* handle, const void *buf, int count);
int vfs_seek(struct VFSEntry* handle, int offset, seek_t origin);
int vfs_tell(struct VFSEntry* handle);
int vfs_eof(struct VFSEntry* handle);


#endif
