#pragma once

#include <stdint.h>

uint16_t vmm_pml4_index(uint64_t virt);
uint16_t vmm_pdpt_index(uint64_t virt);
uint16_t vmm_pd_index(uint64_t virt);
uint16_t vmm_pt_index(uint64_t virt);

void vmm_init(void);