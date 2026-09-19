#include "idt.h"
#include "serial.h"
#include "gdt.h"
#include <stdbool.h>

#define IDT_MAX_DESCRIPTORS 256

__attribute__((aligned(0x10))) 
static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance

static idtr_t idtr;

static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];

extern void isr_stub_0(void);
extern void isr_stub_6(void);


__attribute__((noreturn))
void exception_handler(uint64_t vector) {
    switch (vector) {
        case 0:
            serial_write("EXCEPTION: DIVIDE ERROR\n");
            break;

        case 6:
            serial_write("EXCEPTION: INVALID OPCODE\n");
            break;

        default:
            serial_write("EXCEPTION: UNKNOWN\n");
            break;
    }

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs      = GDT_KERNEL_CODE;
    descriptor->ist            = 0;
    descriptor->attributes     = flags;
    descriptor->isr_mid        = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high       = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved       = 0;
}

void idt_init(void) {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit =
        (uint16_t)(sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1);

    idt_set_descriptor(0, isr_stub_0, 0x8E);
    idt_set_descriptor(6, isr_stub_6, 0x8E);

    __asm__ volatile ("lidt %0" : : "m"(idtr));
}