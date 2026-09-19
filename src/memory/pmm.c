#include "pmm.h"
#include "memory.h"
#include "string.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * One bit represents one physical 4 KiB page.
 *
 * 0 = free
 * 1 = used/reserved
 */
static uint8_t *bitmap = NULL;

static uint64_t total_pages = 0;
static uint64_t bitmap_size = 0;

static uint64_t free_pages = 0;
static uint64_t used_pages = 0;

/*
 * Used only during pmm_init() to reserve contiguous physical
 * memory for the bitmap before the bitmap allocator exists.
 */
static uint64_t bootstrap_next_page = 0;
static uint64_t bootstrap_region_end = 0;


/* ----------------------------------------------------------
 * Bitmap helpers
 * ---------------------------------------------------------- */

static void bitmap_set(uint64_t page) {
    uint64_t byte_index = page / 8;
    uint8_t bit_index = page % 8;

    bitmap[byte_index] |= (uint8_t)(1u << bit_index);
}

static void bitmap_clear(uint64_t page) {
    uint64_t byte_index = page / 8;
    uint8_t bit_index = page % 8;

    bitmap[byte_index] &= (uint8_t)~(1u << bit_index);
}

static bool bitmap_test(uint64_t page) {
    uint64_t byte_index = page / 8;
    uint8_t bit_index = page % 8;

    return (bitmap[byte_index] & (1u << bit_index)) != 0;
}


/* ----------------------------------------------------------
 * Find RAM extent
 * ---------------------------------------------------------- */

static bool is_ram_type(uint64_t type) {
    switch (type) {
        case LIMINE_MEMMAP_USABLE:
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        case LIMINE_MEMMAP_ACPI_NVS:
            return true;

        default:
            return false;
    }
}

static uint64_t find_highest_ram_address(
    struct limine_memmap_response *memmap
) {
    uint64_t highest = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (!is_ram_type(entry->type)) {
            continue;
        }

        uint64_t end = entry->base + entry->length;

        if (end > highest) {
            highest = end;
        }
    }

    return highest;
}


/* ----------------------------------------------------------
 * Bootstrap bitmap allocation
 * ---------------------------------------------------------- */

/*
 * Find one USABLE region large enough to contain the entire
 * bitmap contiguously.
 *
 * Limine guarantees USABLE regions are 4096-byte aligned.
 */
static uint64_t reserve_bitmap_memory(
    struct limine_memmap_response *memmap,
    uint64_t pages_needed
) {
    uint64_t bytes_needed = pages_needed * PAGE_SIZE;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        if (entry->length < bytes_needed) {
            continue;
        }

        bootstrap_next_page = entry->base;
        bootstrap_region_end = entry->base + entry->length;

        uint64_t bitmap_phys = bootstrap_next_page;

        bootstrap_next_page += bytes_needed;

        return bitmap_phys;
    }

    return 0;
}


/* ----------------------------------------------------------
 * PMM initialization
 * ---------------------------------------------------------- */

void pmm_init(struct limine_memmap_response *memmap) {
    if (memmap == NULL) {
        return;
    }

    /*
     * Find how much actual RAM address space we need to
     * represent.
     */
    uint64_t highest_ram_address =
        find_highest_ram_address(memmap);

    total_pages =
        (highest_ram_address + PAGE_SIZE - 1) / PAGE_SIZE;

    bitmap_size =
        (total_pages + 7) / 8;

    uint64_t bitmap_pages =
        (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;

    /*
     * Reserve contiguous physical pages for the bitmap.
     */
    uint64_t bitmap_phys =
        reserve_bitmap_memory(memmap, bitmap_pages);

    if (bitmap_phys == 0) {
        return;
    }

    /*
     * HHDM gives us a virtual address through which we can
     * actually access those physical pages.
     */
    bitmap = (uint8_t *)phys_to_virt(bitmap_phys);

    /*
     * Start by assuming EVERY page is unavailable.
     *
     * 0xFF = 11111111
     */
    memset(bitmap, 0xFF, bitmap_size);

    /*
     * Now mark every page Limine explicitly calls USABLE
     * as free.
     */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry =
            memmap->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        uint64_t start = entry->base;
        uint64_t end = entry->base + entry->length;

        for (uint64_t addr = start;
             addr < end;
             addr += PAGE_SIZE) {

            uint64_t page = addr / PAGE_SIZE;

            if (page < total_pages) {
                bitmap_clear(page);
            }
        }
    }

    /*
     * We just marked all USABLE pages free, including the
     * pages containing our bitmap.
     *
     * Mark those pages used again so the allocator never
     * gives its own bitmap memory away.
     */
    uint64_t bitmap_first_page =
        bitmap_phys / PAGE_SIZE;

    for (uint64_t i = 0; i < bitmap_pages; i++) {
        bitmap_set(bitmap_first_page + i);
    }

    /*
     * Count current free/used pages.
     */
    free_pages = 0;
    used_pages = 0;

    for (uint64_t page = 0; page < total_pages; page++) {
        if (bitmap_test(page)) {
            used_pages++;
        } else {
            free_pages++;
        }
    }
}


/* ----------------------------------------------------------
 * Allocate one physical page
 * ---------------------------------------------------------- */

uint64_t pmm_alloc_page(void) {
    if (bitmap == NULL) {
        return 0;
    }

    for (uint64_t page = 0; page < total_pages; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);

            free_pages--;
            used_pages++;

            return page * PAGE_SIZE;
        }
    }

    /*
     * No physical pages left.
     */
    return 0;
}


/* ----------------------------------------------------------
 * Free one physical page
 * ---------------------------------------------------------- */

void pmm_free_page(uint64_t phys) {
    if (bitmap == NULL) {
        return;
    }

    /*
     * Only page-aligned physical addresses are valid.
     */
    if ((phys & (PAGE_SIZE - 1)) != 0) {
        return;
    }

    uint64_t page = phys / PAGE_SIZE;

    if (page >= total_pages) {
        return;
    }

    /*
     * Don't double-free an already free page.
     */
    if (!bitmap_test(page)) {
        return;
    }

    bitmap_clear(page);

    free_pages++;
    used_pages--;
}


/* ----------------------------------------------------------
 * Statistics
 * ---------------------------------------------------------- */

uint64_t pmm_get_total_pages(void) {
    return total_pages;
}

uint64_t pmm_get_free_pages(void) {
    return free_pages;
}

uint64_t pmm_get_used_pages(void) {
    return used_pages;
}

uint64_t pmm_get_bitmap_size(void) {
    return bitmap_size;
}