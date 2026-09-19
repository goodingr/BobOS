# BobOS

A hobby x86-64 kernel, booted via [Limine](https://github.com/limine-bootloader/limine), built while learning OS development from scratch.

## Building

### Dependencies

- `clang` (targets `x86_64-unknown-none-elf`)
- `ld.lld`
- `nasm`
- `xorriso`
- `qemu-system-x86_64` (to run it)

### 1. Fetch Limine

The bootloader isn't vendored in this repo — clone the binary release branch alongside the kernel source:

```bash
git clone https://github.com/Limine-Bootloader/Limine.git --branch v10.x-binary --depth=1 limine
```

The build also references `limine-protocol` for the `limine.h` protocol header (already copied into `src/limine.h`, so this clone is only needed if you want to diff against upstream or pull in a newer protocol revision):

```bash
git clone https://github.com/Limine-Bootloader/limine-protocol.git --depth=1 limine-protocol
```

### 2. Build and run

```bash
make        # builds the kernel and produces BobbyOS.iso
make run    # builds (if needed) and boots the ISO in QEMU
make clean  # removes build/, iso_root/, and the ISO
```

`make run` launches QEMU with `-serial stdio`, so kernel debug output (see Serial below) prints directly to your terminal.

## Project status

BobOS boots via Limine (protocol base revision 3) into a higher-half x86-64 kernel. What's implemented so far:

- **Framebuffer terminal** — an 8x8 bitmap font renderer with cursor tracking, line wrapping, and scrolling (shifts framebuffer contents up a full line and clears the exposed row).
- **Serial output (COM1)** — used for kernel debug logging; visible via `make run`'s `-serial stdio`.
- **GDT** — a kernel-owned Global Descriptor Table, replacing Limine's bootstrap one.
- **IDT / exceptions** — the table and ISR stub plumbing exist, but only 2 of 32 CPU exception vectors are wired up (divide error, invalid opcode). Hardware interrupts are not yet enabled anywhere (no `sti`), and the legacy PIC has not been remapped.
- **Physical memory manager (PMM)** — a bitmap allocator built from Limine's memory map, with HHDM-based physical/virtual translation.
- **Heap** — a bump allocator (`kmalloc`) on top of the PMM. Currently limited to a single 4 KiB page and cannot grow; there is no `kfree` yet.
- **VMM** — early stage. Can walk the current page tables (PML4 → PDPT → PD → PT) to manually translate a known virtual address to its physical address, as a correctness check. Does not yet support creating new mappings, handling large (2 MiB/1 GiB) pages, or managing its own page tables.

### Not yet implemented

- PIC remapping and hardware interrupt handling (IRQs)
- A keyboard driver
- A growable heap / `kfree`
- A real virtual memory manager (mapping/unmapping arbitrary pages)
- A shell or any form of command input

The near-term goal is working toward a minimal interactive shell, which needs hardware interrupts (PIC + keyboard IRQ) and a line-input buffer on top of the existing terminal before command parsing itself becomes the bottleneck.
