CC = clang
LD = ld.lld
AS = nasm

CFLAGS = \
	-target x86_64-unknown-none-elf \
	-std=gnu11 \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-pic \
	-mno-red-zone \
	-mcmodel=kernel \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-Wall \
	-Wextra \
	-O2 \
	-Isrc \
	-Isrc/arch/x86_64 \
	-Isrc/interrupts \
	-Isrc/memory

LDFLAGS = \
	-m elf_x86_64 \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-T linker.ld

KERNEL = build/kernel.elf
ISO = BobbyOS.iso

.PHONY: all clean run iso

all: $(ISO)

build:
	mkdir -p build

build/main.o: \
	src/main.c \
	src/limine.h \
	src/framebuffer.h \
	src/terminal.h \
	src/string.h \
	src/serial.h \
	src/arch/x86_64/gdt.h \
	src/interrupts/idt.h \
	src/memory/pmm.h \
	src/memory/memory.h \
	src/memory/heap.h \
	src/memory/vmm.h | build
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/framebuffer.o: \
	src/framebuffer.c \
	src/framebuffer.h \
	src/limine.h | build
	$(CC) $(CFLAGS) -c src/framebuffer.c -o build/framebuffer.o

build/font.o: \
	src/font.c \
	src/font.h | build
	$(CC) $(CFLAGS) -c src/font.c -o build/font.o

build/terminal.o: \
	src/terminal.c \
	src/terminal.h \
	src/framebuffer.h \
	src/font.h \
	src/string.h | build
	$(CC) $(CFLAGS) -c src/terminal.c -o build/terminal.o

build/string.o: \
	src/string.c \
	src/string.h | build
	$(CC) $(CFLAGS) -c src/string.c -o build/string.o

build/serial.o: \
	src/serial.c \
	src/serial.h | build
	$(CC) $(CFLAGS) -c src/serial.c -o build/serial.o

build/gdt.o: \
	src/arch/x86_64/gdt.c \
	src/arch/x86_64/gdt.h | build
	$(CC) $(CFLAGS) -c src/arch/x86_64/gdt.c -o build/gdt.o

build/gdt_asm.o: \
	src/arch/x86_64/gdt.S | build
	$(AS) -f elf64 src/arch/x86_64/gdt.S -o build/gdt_asm.o

build/idt.o: \
	src/interrupts/idt.c \
	src/interrupts/idt.h \
	src/arch/x86_64/gdt.h \
	src/serial.h | build
	$(CC) $(CFLAGS) -c src/interrupts/idt.c -o build/idt.o

build/isr.o: \
	src/interrupts/isr.S | build
	$(AS) -f elf64 src/interrupts/isr.S -o build/isr.o

build/pmm.o: \
	src/memory/pmm.c \
	src/memory/pmm.h \
	src/limine.h | build
	$(CC) $(CFLAGS) -c src/memory/pmm.c -o build/pmm.o

build/memory.o: \
	src/memory/memory.c \
	src/memory/memory.h | build
	$(CC) $(CFLAGS) -c src/memory/memory.c -o build/memory.o

build/heap.o: \
	src/memory/heap.c \
	src/memory/heap.h \
	src/memory/pmm.h \
	src/memory/memory.h | build
	$(CC) $(CFLAGS) -c src/memory/heap.c -o build/heap.o

build/vmm.o: \
	src/memory/vmm.c \
	src/memory/vmm.h | build
	$(CC) $(CFLAGS) -c src/memory/vmm.c -o build/vmm.o

$(KERNEL): \
	build/main.o \
	build/framebuffer.o \
	build/font.o \
	build/terminal.o \
	build/string.o \
	build/serial.o \
	build/gdt.o \
	build/gdt_asm.o \
	build/idt.o \
	build/isr.o \
	build/pmm.o \
	build/memory.o \
	build/heap.o \
	build/vmm.o \
	linker.ld

	$(LD) $(LDFLAGS) \
		build/main.o \
		build/framebuffer.o \
		build/font.o \
		build/terminal.o \
		build/string.o \
		build/serial.o \
		build/gdt.o \
		build/gdt_asm.o \
		build/idt.o \
		build/isr.o \
		build/pmm.o \
		build/memory.o \
		build/heap.o \
		build/vmm.o \
		-o $(KERNEL)

iso: $(ISO)

$(ISO): $(KERNEL) limine.conf
	rm -rf iso_root
	mkdir -p iso_root/boot/limine
	mkdir -p iso_root/EFI/BOOT

	cp $(KERNEL) iso_root/boot/kernel.elf
	cp limine.conf iso_root/boot/limine/limine.conf

	cp limine/limine-bios.sys iso_root/boot/limine/
	cp limine/limine-bios-cd.bin iso_root/boot/limine/
	cp limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp limine/BOOTX64.EFI iso_root/EFI/BOOT/

	xorriso \
		-as mkisofs \
		-R \
		-r \
		-J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		iso_root \
		-o $(ISO)

	./limine/limine bios-install $(ISO)

run: $(ISO)
	qemu-system-x86_64 \
		-M q35 \
		-m 256M \
		-cdrom $(ISO) \
		-serial stdio

clean:
	rm -rf build
	rm -rf iso_root
	rm -f $(ISO)