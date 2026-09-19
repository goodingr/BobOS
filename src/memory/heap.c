#include "heap.h"
#include "memory.h"
#include "pmm.h"


#define ALIGNMENT 16

typedef struct {
    uint64_t base;
    uint64_t size;
} allocation_t;

static uint64_t next_free;
static size_t next_index;
static uint64_t page;


allocation_t allocations[256];

void heap_init(void) {
    next_index = 0;
    page = (uint64_t)phys_to_virt(pmm_alloc_page());
    next_free = page;
}

void *kmalloc(size_t size) {
    if (next_free + size - page > PAGE_SIZE) {
        return NULL;
    }
    // Create the new allocation and add it to array
    allocation_t new_allocation = { 
        .base = next_free, 
        .size = size };

    allocations[next_index] = new_allocation;
    next_index++;
    
    uint64_t addr = next_free;
    // Increase next free, keeping alignment
    next_free = addr + size;
    next_free = (next_free + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    return (void*)addr;
}