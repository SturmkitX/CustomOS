#include "../drivers/screen.h"
#include "../drivers/ata_pio_drv.h"
#include <stdint.h>

#define DISK_START_ADDR     0x00200000
#define KERNEL_START_ADDR   0x00203000  // at the start of the 25th sector

void stage2_main() {
    kprint("Entered Stage 2\n");

    uint8_t diskstatus = identify();
    kprintf("Disk present: %u\n", diskstatus);

    // Allocate buffer for first 127 sectors on disk (until we reach FAT32)
    uint8_t* diskData = (uint8_t*) DISK_START_ADDR;

    // Read disk (FAT32 structure is broken now)
    ata_pio_read48(0, 1024, diskData);

    kprintf("Disk Data first 5 sectors (1st sector): %x %x %x %x %x\n", diskData[0], diskData[1], diskData[2], diskData[3], diskData[4]);
    kprintf("Disk Data first 5 sectors (2nd sector): %x %x %x %x %x\n", diskData[512], diskData[513], diskData[514], diskData[515], diskData[516]);

    // cast the Kernel start address to a void function pointer, then call the function (basically enter kernel)
    void (*kernel_entry)() = KERNEL_START_ADDR;
    (*kernel_entry)();
}