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
#include "../drivers/arp.h"
#include "../cpu/pci.h"

#include "../libc/endian.h"
#include "../drivers/icmp.h"
#include "../drivers/udp.h"
#include "../drivers/dns.h"
#include "../drivers/tcp.h"

static char _k_kbd_buff[256];

void kernel_main() {
    isr_install();
    irq_install();

    asm("int $2");
    asm("int $3");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");

    // Ugly way to implement it, must implement threads
    while (1) {
        if (strlen(_k_kbd_buff) == 0) {
            continue;
        }

        if (strcmp(_k_kbd_buff, "END") == 0) {
            kprint("Stopping the CPU. Bye!\n");
            asm volatile("hlt");
        } else if (strcmp(_k_kbd_buff, "PAGE") == 0) {
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
        } else if (strcmp(_k_kbd_buff, "PAGE2") == 0) {
            uint32_t i;
            kprint("Trying with FAT size!\n");
            uint32_t size = 512 * 4075;
            uint8_t* buff = (uint8_t*) kmalloc(size);
            for (i=0; i < size; i++) {
                buff[i] = 0xab;
            }
        } else if (strcmp(_k_kbd_buff, "TEST") == 0) {
            kprint("Trying out our new INT!\n");
            asm("int $0x90");
        } else if (strcmp(_k_kbd_buff, "FS") == 0) {
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
        } else if (strcmp(_k_kbd_buff, "DISK") == 0) {
            kprint("Checking disk presence...\n");
            uint8_t present = identify();

            kprintf("Disk present: %u\n", present);
        } else if (strcmp(_k_kbd_buff, "SER") == 0) {
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
        } else if (strcmp(_k_kbd_buff, "PCI") == 0) {
            kprint("Checking PCI...\n");

            // checkAllBuses();
            printHeaderBytes(0, 3, 0);
        } else if (strcmp(_k_kbd_buff, "NET") == 0) {
            kprint("Checking RTL8139 status...\n");

            uint32_t ioaddr = identifyRTL8139();
            kprintf("Received IO Addr %u\n", ioaddr);

            uint8_t* macAddr = getMACAddress();
            kprintf("NET MAC Address: %x:%x:%x:%x:%x:%x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

            kfree(macAddr);
            
            initializeRTL8139();

            
        } else if (strcmp(_k_kbd_buff, "NET2") == 0) {
            kprint("Trying to send a packet...\n");

            // construct ARP
            struct ARP arp;
            union IPAddress gwAddr;
            gwAddr.integerForm = 167772674;     // 10.0.2.2

            // uint32_t arpi;
            // for (arpi=0; arpi < 32; arpi++) {
                // constructARP(&arp, &gwAddr);
                // kprintf("ARP size: %u\n", sizeof(struct ARP));
                // convertARPEndianness(&arp);
                // transmit_packet(&arp, sizeof(struct ARP));

                kprint("Sent ARP packet from 10.0.2.15 (us) to 10.0.2.2 (Gateway)\n");
            // }
            
        } else if (strcmp(_k_kbd_buff, "ICMP") == 0) {
            kprint("Trying to send ICMP packet to 8.8.8.8\n");
            union IPAddress target_ip;
            // target_ip.integerForm = 3232238081;
            target_ip.integerForm = 134744072;

            struct ICMPEchoPacket icmp;
            constructICMPEcho(&icmp, &target_ip, 1);
            convertICMPEchoEndianness(&icmp);

            kprintf("ICMP Echo size: %u\n", sizeof(struct ICMPEchoPacket));
            transmit_packet(&icmp, sizeof(struct ICMPEchoPacket));

            kprint("Sent ICMP packet from 10.0.2.15 (us) to 8.8.8.8 (Google DNS)\n");
            
        } else if (strcmp(_k_kbd_buff, "UDP") == 0) {
            kprint("Trying to send UDP packet to 192.168.10.105\n");
            union IPAddress target_ip;
            // target_ip.integerForm = 3232238081;
            target_ip.integerForm = 3232238185;

            struct UDPPacket udp;
            char *udpPayload = "Un mesaj dragut prin UDP.";
            constructUDPHeader(&udp, &target_ip, 50000, 8080, udpPayload, strlen(udpPayload));

            kprintf("UDP size: %u\n", udp.total_length);
            sendUDP(&udp, udpPayload, strlen(udpPayload));

            kprint("Sent UDP packet from 10.0.2.15 (us) to 192.168.10.105\n");
            
        } else if (strcmp(_k_kbd_buff, "DNS") == 0) {
            kprint("Trying to send DNS packet to 192.168.10.1 (built-in)\n");

            struct DNSPacket dns;
            char *dnsName = "www.facebook.com";
            constructDNSHeader(&dns, dnsName, strlen(dnsName));

            sendDNS(&dns, dnsName, strlen(dnsName));

            kprint("Sent DNS packet for www.facebook.com to 192.168.10.1\n");
            
        } else if (strcmp(_k_kbd_buff, "TCP") == 0) {
            kprint("Trying to send TCP packet to 192.168.10.1\n");
            union IPAddress target_ip;
            // target_ip.integerForm = 3232238081;
            target_ip.integerForm = 3232238081;

            struct TCPPacket tcp;
            // char *tcpPayload = "Un mesaj dragut prin UDP.";
            constructTCPHeader(&tcp, &target_ip, 50002, 80, NULL, 0, 1, 0);

            // kprintf("UDP size: %u\n", udp.total_length);
            sendTCP(&tcp, NULL, 0);

            // wait for the response (disregard IP for now)
            kprint("Waiting for SYN response...");
            struct TCPPacket* syn_res = NULL;
            do {
                syn_res = pollTCP(50002);
            } while (syn_res == NULL);

            kprint("Sent TCP packet from 10.0.2.15 (us) to 192.168.10.1\n");
            if ((syn_res->offset_and_flags & TCP_FLAG_SYN) && (syn_res->offset_and_flags & TCP_FLAG_ACK)) {
                kprint("Received Handshake response... Sending ACK");
                struct TCPPacket respTCP;
                constructTCPHeader(&respTCP, &target_ip, 50002, 80, NULL, 0, 0, 1);
                sendTCP(&respTCP, NULL, 0);
            }
        }
        kprint("You said: ");
        kprint(_k_kbd_buff);

        char s[3];
        int_to_ascii(TestIntNumber, s);
        kprint(s);
        kprint("\n");
        kprint("\n> ");

        _k_kbd_buff[0] = '\0';
    }
}

void user_input(char *input) {
    uint32_t len = strlen(input);
    memory_copy(_k_kbd_buff, input, len);
    _k_kbd_buff[len] = '\0';
}
