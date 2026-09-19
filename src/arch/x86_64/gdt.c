#include "gdt.h"

static gdt_entry_t gdt[3];
static gdtr_t gdtr;

extern void gdt_load(gdtr_t *gdtr);

void gdt_init(void) {
    gdt[0] = (gdt_entry_t){0};
    gdt[1].limit_low    = 0xFFFF;
    gdt[1].base_low     = 0x0000;
    gdt[1].base_middle  = 0x00;
    gdt[1].access       = GDT_KERNEL_CODE_ACCESS;
    gdt[1].granularity  = GDT_KERNEL_CODE_GRANULARITY;
    gdt[1].base_high    = 0x00;
    
    gdt[2].limit_low   = 0xFFFF;
    gdt[2].base_low    = 0x0000;
    gdt[2].base_middle = 0x00;
    gdt[2].access      = GDT_KERNEL_DATA_ACCESS;
    gdt[2].granularity = GDT_KERNEL_DATA_GRANULARITY;
    gdt[2].base_high   = 0x00;

    gdtr.base = (uint64_t)&gdt[0];
    gdtr.limit = sizeof(gdt) - 1;   

    gdt_load(&gdtr);
}
