#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "kernel.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/globals.h"
#include <stdint.h>

#include "../drivers/fat32.h"
#include "../drivers/ata_pio_drv.h"

void kernel_main() {
    isr_install();
    irq_install();

    asm("int $2");
    asm("int $3");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");
}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    } else if (strcmp(input, "PAGE") == 0) {
        /* Lesson 22: Code to test kmalloc, the rest is unchanged */
        uint32_t phys_addr;
        uint32_t page = (uint32_t) kmalloc2(1000, 1, &phys_addr);
        char page_str[16] = "";
        hex_to_ascii(page, page_str);
        char phys_str[16] = "";
        hex_to_ascii(phys_addr, phys_str);
        kprint("Page: ");
        kprint(page_str);
        kprint(", physical address: ");
        kprint(phys_str);
        kprint("\n");
    } else if (strcmp(input, "PAGE2") == 0) {
        uint32_t i;
        kprint("Trying with FAT size!\n");
        uint32_t size = 512 * 4075;
        uint8_t* buff = (uint8_t*) kmalloc(size);
        for (i=0; i < size; i++) {
            buff[i] = 0xab;
        }
    } else if (strcmp(input, "TEST") == 0) {
        kprint("Trying out our new INT!\n");
        asm("int $0x90");
    } else if (strcmp(input, "FS") == 0) {
        kprint("Opening file system (FAT32)!\n");
        f32* fat32 = makeFilesystem(NULL);

        if (fat32 == NULL) {
            kprint("FAT32 is NULL. FAIL");
            return;
        } else {
            kprint("FAT32 found. SUCCESS!");
        }

        struct directory dir;
        populate_root_dir(fat32, &dir);

        if (dir.entries != NULL) {
            kprint("Entries have been populated.\n");
        } else {
            kprint("Entries are empty...");
            return;
        }

        // kprint("Creating a new directory...");
        // mkdir(fat32, &dir, "GENERA");

        kprintf("Printing FAT contents (items = %d)...\n", dir.num_entries);
        print_directory(fat32, &dir);
    } else if (strcmp(input, "DISK") == 0) {
        kprint("Checking disk presence...\n");
        uint8_t present = identify();

        kprintf("Disk present: %u\n", present);
    }
    kprint("You said: ");
    kprint(input);

    char s[3];
    int_to_ascii(TestIntNumber, s);
    kprint(s);
    kprint("\n");
    kprint("\n> ");
}
