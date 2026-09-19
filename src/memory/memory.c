#include "memory.h"

static uint64_t hhdm_offset;

void memory_set_hhdm(uint64_t offset) {
    hhdm_offset = offset;
}

void *phys_to_virt(uint64_t phys) {
    return (void *)(phys + hhdm_offset);
}

uint64_t virt_to_phys(void *virt) {
    return (uint64_t)virt - hhdm_offset;
}