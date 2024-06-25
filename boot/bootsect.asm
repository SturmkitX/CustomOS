; Identical to lesson 13's boot sector, but the %included files have new paths
[org 0x7c00]
STAGE2_OFFSET equ 0x1000 ; The same one we used when linking the kernel

    mov [BOOT_DRIVE], dl ; Remember that the BIOS sets us the boot drive in 'dl' on boot
    mov bp, 0x9000
    mov sp, bp

    mov bx, MSG_REAL_MODE 
    call print
    call print_nl

    call load_kernel ; read the kernel from disk
    call switch_to_pm ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'
    jmp $ ; Never executed

%include "boot/print.asm"
%include "boot/print_hex.asm"
%include "boot/disk.asm"
%include "boot/gdt.asm"
%include "boot/32bit_print.asm"
%include "boot/switch_pm.asm"

[bits 16]
load_kernel:
    mov bx, MSG_LOAD_KERNEL
    call print
    call print_nl

    mov bx, STAGE2_OFFSET ; Read from disk and store in 0x1000
    mov dh, 24 ; Our future kernel will be larger, make this big; ATTENTION: Current Kernel is at least 56
    ; sectors long and we risk getting errors. We should somehow load the contents of the floppy from the
    ; protected 32-bit mode. If I increase dh past 52, the bootloader hangs when reading the floppy

    mov ax, 0x4f02  ; VESA command
    mov bx, 0x4112  ; 640x480, 24 bit color (RGB), linear framebuffer (0x4000 ored)
    int 0x10

    ; mov ax, 0x0013
    ; int 0x10

    mov dl, [BOOT_DRIVE]
    call disk_load
    ret

[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    call STAGE2_OFFSET ; Give control to the kernel
    jmp $ ; Stay here when the kernel returns control to us (if ever)


BOOT_DRIVE db 0x80 ; It is a good idea to store it in memory because 'dl' may get overwritten
MSG_REAL_MODE db "Started in 16-bit Real Mode", 0
MSG_PROT_MODE db "Landed in 32-bit Protected Mode", 0
MSG_LOAD_KERNEL db "Loading Stage2 into memory", 0
MSG_RETURNED_KERNEL db "Returned from Stage2. Error?", 0

; padding
times 510 - ($-$$) db 0
dw 0xaa55
