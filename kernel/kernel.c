#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "kernel.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/globals.h"
#include <stdint.h>

#include "../drivers/fat32.h"
#include "../drivers/ata_pio_drv.h"
#include "../drivers/serial.h"
#include "../drivers/rtl8139.h"
#include "../cpu/pci.h"

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
        kprintf("Item 6 name: %s\n", dir.entries[5].name);

        char* readmeContents = (char*) readFile(fat32, dir.entries + 5);
        kprintf("README.txt contents: %s\n", readmeContents);

        struct directory subdir;
        populate_dir(fat32, &subdir, dir.entries[3].first_cluster);
        kprintf("Printing Folder1 contents (items = %d)...\n", subdir.num_entries);
        print_directory(fat32, &subdir);
        char* notesContents = (char*) readFile(fat32, subdir.entries + 4);
        kprintf("Folder1 Notes.txt contents: %s\n", notesContents);

        // char* writeContents = "Baga-mi-as pula-n gura ta de zdreanta, de muista. Pisa-mi-ai pe fata ta, de zdrente de femei\r\n";
        // struct directory newsubdir;
        // populate_dir(fat32, &newsubdir, dir.entries[2].first_cluster);
        // writeFile(fat32, &newsubdir, writeContents, "deschide.txt", strlen(writeContents));
        // kprint("Created new file in GENERA folder. Check it out.");

        // free_directory(fat32, &newsubdir);
        free_directory(fat32, &subdir);
        free_directory(fat32, &dir);

        kfree(notesContents);
    } else if (strcmp(input, "DISK") == 0) {
        kprint("Checking disk presence...\n");
        uint8_t present = identify();

        kprintf("Disk present: %u\n", present);
    } else if (strcmp(input, "SER") == 0) {
        kprint("Checking serial comms...\n");
        uint8_t serStatus = init_serial();

        kprintf("Serial init status: %u\n", serStatus);

        // Test send this string periodically
        uint32_t index = 0;
        char* testMessage = "How are you man?\r\n";
        while (1) {
            index++;
            if (index % 100000000 == 0) {
                kprint("Printing test message to serial\n");
                write_string_serial(testMessage);
            }
        }
    } else if (strcmp(input, "PCI") == 0) {
        kprint("Checking PCI...\n");

        // checkAllBuses();
        printHeaderBytes(0, 3, 0);
    } else if (strcmp(input, "NET") == 0) {
        kprint("Checking RTL8139 status...\n");

        uint32_t ioaddr = identifyRTL8139();
        kprintf("Received IO Addr %u\n", ioaddr);

        uint8_t* macAddr = getMACAddress(ioaddr);
        kprintf("NET MAC Address: %x:%x:%x:%x:%x:%x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

        kfree(macAddr);
        uint8_t rtlIRQ = getIRQNumber(0x10EC, 0x8139);
        kprintf("RTL IRQ Number: %u\n", rtlIRQ);
        // initializeRTL8139(ioaddr);
    }
    kprint("You said: ");
    kprint(input);

    char s[3];
    int_to_ascii(TestIntNumber, s);
    kprint(s);
    kprint("\n");
    kprint("\n> ");
}
