#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"
#include "framebuffer.h"
#include "terminal.h"
#include "string.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "memory.h"
#include "heap.h"
#include "vmm.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = { 
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, 
    .revision = 0 
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
    for (;  ;) {
        asm ("hlt");
    }
}

void parse_memory_map(void) {
    struct limine_memmap_response *memmap_response = memmap_request.response;

    if (memmap_response == NULL) {
        // Handle error: no memory map response received
        return;
    }

    // Iterate through the entries
    for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_response->entries[i];
        
        // Access entry->base, entry->length, and entry->type
        // Types include LIMINE_MEMMAP_USABLE, LIMINE_MEMMAP_RESERVED, etc.
        serial_write("MEMMAP ENTRY: Base ");
        char str[17];
        uint64_to_hex(entry->base, str);
        serial_write(str);
        serial_write(" Length ");
        uint64_to_hex(entry->length, str);
        serial_write(str);
        serial_write(" Type ");
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                serial_write("LIMINE_MEMMAP_USABLE");
                break;
            case LIMINE_MEMMAP_RESERVED:
                serial_write("LIMINE_MEMMAP_RESERVED");
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                serial_write("LIMINE_MEMMAP_ACPI_RECLAIMABLE");
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                serial_write("LIMINE_MEMMAP_ACPI_NVS");
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                serial_write("LIMINE_MEMMAP_BAD_MEMORY");
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                serial_write("LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE");
                break;
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
                serial_write("LIMINE_MEMMAP_EXECUTABLE_AND_MODULES");
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                serial_write("LIMINE_MEMMAP_FRAMEBUFFER");
                break;
            case LIMINE_MEMMAP_RESERVED_MAPPED:
                serial_write("LIMINE_MEMMAP_RESERVED_MAPPED");
                break;
        }
        serial_write("\n");
    }
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    framebuffer_init(framebuffer_request.response->framebuffers[0]);
    clear_screen(0x00102030);

    terminal_init(0x00337788, 0x00FFFFFF);
    terminal_write("Terminal initialized\n");

    if (serial_init() == 0) {
        serial_write("Serial initialized\n");
        terminal_write("Serial initialized\n");
    }    

    gdt_init();
    serial_write("GDT loaded\n");
    idt_init();
    serial_write("IDT loaded\n");

    parse_memory_map();

    if (hhdm_request.response == NULL) {
        hcf();
    }
    memory_set_hhdm(hhdm_request.response->offset);

    pmm_init(memmap_request.response);

    uint64_t bitmap_size = pmm_get_bitmap_size();
    uint64_t total_pages = pmm_get_total_pages();

    char str[21];
    serial_write("\nTotal Pages: ");
    uint64_to_string(total_pages, str);
    serial_write(str);
    serial_write("\nBitmap Size: ");
    uint64_to_string(bitmap_size, str);
    serial_write(str);

    uint64_t page1 = pmm_alloc_page();
    uint64_t page2 = pmm_alloc_page();

    uint64_t *ptr1 = phys_to_virt(page1);
    uint64_t *ptr2 = phys_to_virt(page2);

    ptr1[0] = 0x1111111111111111;
    ptr2[0] = 0x2222222222222222;

    if (ptr1[0] == 0x1111111111111111 &&
        ptr2[0] == 0x2222222222222222) {
        serial_write("\nPMM allocation test passed\n");
    }

    pmm_free_page(page1);

    uint64_t page3 = pmm_alloc_page();

    if (page3 == page1) {
        serial_write("PMM free/reuse test passed\n");
    }
    // volatile int a = 10;
    // volatile int b = 0;
    // volatile int c = a / b;
    // (void)c;

    heap_init();

    uint64_t *a = kmalloc(sizeof(uint64_t));
    uint64_t *b = kmalloc(sizeof(uint64_t));

    *a = 123;
    *b = 456;

    if (*a == 123 && *b == 456) {
        serial_write("Heap test passed\n");
    }

    vmm_init();


    hcf();
}