#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER     (1ULL << 2)

uint16_t vmm_pml4_index(uint64_t virt);
uint16_t vmm_pdpt_index(uint64_t virt);
uint16_t vmm_pd_index(uint64_t virt);
uint16_t vmm_pt_index(uint64_t virt);

void vmm_init(void);

bool vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);