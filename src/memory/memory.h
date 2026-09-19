#pragma once
#include <stdint.h>

void memory_set_hhdm(uint64_t offset);
void *phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(void *virt);