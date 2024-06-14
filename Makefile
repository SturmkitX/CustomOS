C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c boot_stage2/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h)
# Nice syntax for file extension replacement
OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o} 

# Change this if your cross-compiler is somewhere else
CC = i686-elf-gcc
GDB = i686-elf-gdb
# -g: Use debugging symbols in gcc
CFLAGS = -g -ffreestanding -Wall -Wextra -fno-exceptions -m32

DDPATH = "C:\Users\Bogdan Rogoz\Desktop\os-dev\w64devkit\bin\dd.exe"
#DDPATH = dd

# all: boot/bootsect.bin kernel.bin stage2.bin
# 	${DDPATH} conv=notrunc if=/dev/zero of=hdd1-raw2.img bs=1b count=127
# 	${DDPATH} conv=notrunc if=boot/bootsect.bin of=hdd1-raw2.img bs=1b
# 	${DDPATH} conv=notrunc if=stage2.bin of=hdd1-raw2.img bs=1b seek=1
# 	${DDPATH} conv=notrunc if=kernel.bin of=hdd1-raw2.img bs=1b seek=24

# music.raw has 60496 sectors
all: boot/bootsect.bin kernel.bin stage2.bin music.raw
	${DDPATH} conv=notrunc if=boot/bootsect.bin of=hdda.img bs=1b
	${DDPATH} conv=notrunc if=stage2.bin of=hdda.img bs=1b seek=1
	${DDPATH} conv=notrunc if=kernel.bin of=hdda.img bs=1b seek=24
	${DDPATH} conv=notrunc if=music.raw of=hdda.img bs=4K seek=256

testapp.bin: app_test/app_entry.o app_test/app_main.o drivers/screen.o libc/mem.o libc/string.o cpu/ports.o
	i686-elf-ld -o $@ -Ttext 0x300000 $^ --oformat binary

# First rule is run by default
os-image.bin: boot/bootsect.bin stage2.bin
#	cat $^ > os-image.bin
	${DDPATH} if=/dev/zero of=os-image.bin bs=1b count=2880
	${DDPATH} conv=notrunc if=boot/bootsect.bin of=os-image.bin bs=1b
	${DDPATH} conv=notrunc if=stage2.bin of=os-image.bin bs=1b seek=1

# '--oformat binary' deletes all symbols as a collateral, so we don't need
# to 'strip' them manually on this case
kernel.bin: boot/kernel_entry.o ${OBJ}
	i686-elf-ld -o $@ -Ttext 0x00203000 $^ --oformat binary

stage2.bin: boot_stage2/stage2.o drivers/screen.o libc/mem.o libc/string.o cpu/ports.o boot_stage2/stage2_entry.o drivers/ata_pio_drv.o
	i686-elf-ld -o $@ -Ttext 0x1000 $^ --oformat binary

# Used for debugging purposes
kernel.elf: boot/kernel_entry.o ${OBJ}
	i686-elf-ld -o $@ -Ttext 0x1000 $^ 

run:
#	qemu-system-x86_64 -hda hdd1-raw2.img -m 512M -nic user,model=rtl8139
	qemu-system-x86_64 -hda hdda.img -m 512M -device rtl8139,netdev=u1 -netdev user,id=u1 -object filter-dump,id=f1,netdev=u1,file=dump.dat -audio driver=dsound,model=ac97

# Open the connection to qemu and load our kernel-object file with symbols
debug: os-image.bin kernel.elf
	qemu-system-x86_64 -s -fda os-image.bin -d guest_errors,int &
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

# Generic rules for wildcards
# To make an object, always compile from its .c
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image.bin *.elf
	rm -rf kernel/*.o boot/*.bin drivers/*.o boot/*.o cpu/*.o libc/*.o boot_stage2/*.o
