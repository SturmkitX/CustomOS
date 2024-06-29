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

#include "../drivers/ac97.h"
#include "../cpu/timer.h"
#include "../drivers/e1000.h"
#include "../drivers/vga.h"

#include "../libc/function.h"
#include "../drivers/vfs.h"
#include "doom.h"
#include "midi.h"

#include "../fdlibm/fdlibm.h"

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

            uint8_t netExists = identifyRTL8139();
            kprintf("RTL8139 Present: %u\n", netExists);

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
            // convertICMPEchoEndianness(&icmp);

            // kprintf("ICMP Echo size: %u\n", sizeof(struct ICMPEchoPacket));
            // transmit_packet(&icmp, sizeof(struct ICMPEchoPacket));
            sendICMPEcho(&icmp);

            kprint("Sent ICMP packet from 10.0.2.15 (us) to 8.8.8.8 (Google DNS)\n");
            
        } else if (strcmp(_k_kbd_buff, "UDP") == 0) {
            kprint("Trying to send UDP packet to 192.168.10.105\n");
            union IPAddress target_ip;
            // target_ip.integerForm = 3232238081;
            target_ip.integerForm = 3232238203;

            struct UDPPacket udp;
            char *udpPayload = "Un mesaj dragut prin UDP.";
            constructUDPHeader(&udp, &target_ip, 50000, 8080, udpPayload, strlen(udpPayload));

            kprintf("UDP size: %u\n", udp.total_length);
            sendUDP(&udp);

            // wait for UDP response
            kprint("Waiting for UDP response...\n");
            struct UDPPacket* resp = NULL;
            do {
                resp = pollUDP(50000);
            } while (resp == NULL);

            kprintf("\nGot UDP response packet: Len = %u, Body = %s\n\n", resp->payloadSize, resp->payload);

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
            target_ip.integerForm = 3232238203;

            struct TCPPacket tcp;
            // char *tcpPayload = "Un mesaj dragut prin UDP.";
            constructTCPHeader(&tcp, &target_ip, 50002, 12345, NULL, 0, 1, 0, 0, 0);

            // kprintf("UDP size: %u\n", udp.total_length);
            sendTCP(&tcp);

            // wait for the connection (disregard IP for now)
            kprint("Waiting for TCP Connection...");

            while(!checkTCPConnection(50002)) {tcpTXBufferHandler();}    // do nothing
            uint32_t bufi;   // may need to call it multiple times, just in case (it should be safe)

            for (bufi = 0; bufi < 15; bufi++) {
                tcpTXBufferHandler();
            }

            kprint("Established connection to 192.168.10.1:80\n");
            // we should now send our request
            char* httpReq = "GET / HTTP/1.1\r\nHost: 192.168.10.1\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.9,en-US;q=0.8,en;q=0.7,de;q=0.6,el;q=0.5\r\n\r\n";
            
            kprint("Attempting to send a HTTP request...\n");
            struct TCPPacket httpTCP;
            constructTCPHeader(&httpTCP, &target_ip, 50002, 12345, httpReq, strlen(httpReq), 0, 1, 1, 0);
            sendTCP(&httpTCP);
            for (bufi = 0; bufi < 15; bufi++) {
                tcpTXBufferHandler();
            }

            uint32_t currentTick = get_ticks();    // quick hack to access current timer tick (a tick occurs every 20ms right now)
            while (get_ticks() - currentTick < 100) {tcpTXBufferHandler();}    // perform all possible transmissions (responses) in the next 2 seconds

            kprintf("Current TCP Poll size: %u\n", getTCPBufferSize(50002));
            struct TCPPacket* respHTTP = pollTCP(50002);
            do {
                // if (respHTTP->payloadSize > 0)
                    kprintf("%s %u", respHTTP->payload, respHTTP->payloadSize);
                respHTTP = pollTCP(50002);
            } while (respHTTP != NULL);

        } else if (strcmp(_k_kbd_buff, "AUDIO") == 0) {
            kprint("Checking AC97 status...\n");

            uint8_t audioExists = identifyAC97();
            kprintf("AC97 Present: %u\n", audioExists);
            
            initializeAC97(11025);
        } else if (strcmp(_k_kbd_buff, "PLAY") == 0) {
            kprint("Trying to play some audio...\n");
            kprint("Starting with garbage!\n");

            uintptr_t garbage = (uintptr_t) kmalloc(15726 * 512);

            kprint("Garbage allocated\n");

            struct VFSEntry* vfs = vfs_open("betray.raw", "rb");
            if (vfs) {
                kprint("Found 11KHz song\n");
                vfs_read(vfs, garbage, 15726 * 512);
            }
            

            // let's test if working with max 255 sectors will improve the stability (no)
            // uint16_t sectori = 0;
            // for (sectori = 0; sectori < 32000; sectori += 200) {
            //     ata_pio_read48(256 + sectori, 200, garbage + sectori * 512);
            // }

            kprintf("%u %u %u\n", *(uint8_t*)(garbage), *(uint8_t*)(garbage + 1), *(uint8_t*)(garbage + 2000));
            
            playAudio(garbage, 2048 * 30);
        } else if (strcmp(_k_kbd_buff, "SLEEP") == 0) {
            kprint("Starting 5 second sleep...\n");

            sleep(30000);
            kprintf("Done sleeping!\n");
        } else if (strcmp(_k_kbd_buff, "E1000") == 0) {
            kprint("Checking E1000 status...\n");

            uint8_t netExists = identifyE1000();
            kprintf("E1000 Present: %u\n", netExists);
            
            initializeE1000();

            // Get MAC Address
            uint8_t* macAddr = E1000GetMACAddress();
            kprintf("NET MAC Address: %x:%x:%x:%x:%x:%x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        } else if (strcmp(_k_kbd_buff, "VFS1") == 0) {
            struct VFSEntry* vfs = vfs_open("test1.txt", "wa");
            if (vfs != NULL) {
                kprint("Write file not empty\n");
                char* msg = "Ce faci mai pula?";
                uint32_t wr = vfs_write(vfs, msg, strlen(msg));
                kprintf("Written characters: %u\n", wr);
            }
        } else if (strcmp(_k_kbd_buff, "VFS2") == 0) {
            struct VFSEntry* vfs = vfs_open("test2.txt", "wa");
            if (vfs != NULL) {
                kprint("Write file not empty\n");
                char* msg = "Al 2lea fisier";
                uint32_t wr = vfs_write(vfs, msg, strlen(msg));
                kprintf("Written characters: %u\n", wr);
            }
        } else if (strcmp(_k_kbd_buff, "VFS3") == 0) {
            struct VFSEntry* vfs = vfs_open("WADS/doom1.wad", "r");
            if (vfs != NULL) {
                kprint("Reading from file\n");
                char msg[20];

                vfs_seek(vfs, 0x3FDA3C, SEEK_SET);
                uint32_t readb = vfs_read(vfs, msg, 10);
                kprintf("Read Bytes = %u, EOF = %u\n", readb, vfs_eof(vfs));

                readb = vfs_read(vfs, msg + 10, 20);
                kprintf("Read Bytes = %u, EOF = %u\n", readb, vfs_eof(vfs));    // should read 4 bytes, what is left

                msg[14] = 0;
                kprintf("Message is %s\n", msg);
            }
        } else if (strcmp(_k_kbd_buff, "VFS4") == 0) {
            struct VFSEntry* vfs = vfs_open("test3.txt", "wa");
            if (vfs != NULL) {
                kprint("Testing some sick seeks\n");

                char msg[20];

                vfs_write(vfs, "Radullescu", 10);
                vfs_read(vfs, msg, 20);
                kprintf("First read: %s\n", msg);

                vfs_seek(vfs, 0, SEEK_SET);
                vfs_write(vfs, "Jonah", 6);

                vfs_seek(vfs, 0, SEEK_SET);
                vfs_read(vfs, msg, 20);
                kprintf("Second read: %s\n", msg);
            }
        } else if (strcmp(_k_kbd_buff, "VGA") == 0) {
            kprint("Trying to put pixel in VGA\n");

            // init_vga();
            uintptr_t garbage = (uintptr_t) kmalloc(921600 * 2);  // 24 bit color JPG, 900 KB (loading screen + main photo)

            kprint("Garbage allocated\n");

            kprint("Loading the picture...\n");
            ata_pio_read48(256, 3600, garbage);

            draw_icon(0, 0, 640, 480, garbage);
            // fill_rect(0, 0, 320, 80, 0x00FF0000);

            // fill_rect(0, 0, 640, 480, 0x00FF0000);
            // uint32_t i,j;
            // for (i=0; i < 480; i++) {
            //     for (j=0; j < 640; j++) {
            //         putpixel(j, i, 0x0000FF00);
            //         sleep(1);
            //     }
            // }

            uintptr_t music = (uintptr_t) kmalloc(63428856);  // 123884 sectors, ~60 MB

            kprint("Music allocated\n");

            uint32_t readSec = 0;
            while (readSec < 123884) {
                uint32_t toread = MIN(123884 - readSec, 32000);
                ata_pio_read48(4000 + readSec, toread, music + readSec * 512);
                readSec += toread;
            }

            // Change to main photo and play music
            draw_icon(0, 0, 640, 480, garbage + 1800 * 512);

            playAudio(music, 63428856);
        } else if (strcmp(_k_kbd_buff, "DOOM") == 0) {
            kprint("Initializing our doom...\n");
            initialize_doom();
        } else if (strcmp(_k_kbd_buff, "MATH") == 0) {
            kprint("Trying out some math\n");
            double result = sqrt(16);

            kprintf("The result is %u\n", (int)result);
        } else if (strcmp(_k_kbd_buff, "MIDI") == 0) {
            kprint("Playing E1M1 music\n");

            play_midi();
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
