#pragma once
#include <stdint.h>

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_CODE_ACCESS 0x9A
#define GDT_KERNEL_CODE_GRANULARITY 0xAF
#define GDT_KERNEL_DATA 0x10
#define GDT_KERNEL_DATA_ACCESS 0x92
#define GDT_KERNEL_DATA_GRANULARITY 0xCF

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;

} __attribute__((packed)) gdt_entry_t;

_Static_assert(sizeof(gdt_entry_t) == 8,
               "GDT entry must be 8 bytes");

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdtr_t;

_Static_assert(sizeof(gdtr_t) == 10,
               "GDTR must be 10 bytes");

void gdt_init(void);