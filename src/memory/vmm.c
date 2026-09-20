#include "vmm.h"
#include "memory.h"
#include "serial.h"
#include "string.h"
#include "pmm.h"


typedef uint64_t page_table_entry_t;
typedef page_table_entry_t page_table_t[512];
static page_table_t *pml4;

static uint64_t read_cr3(void) {
    uint64_t value;

    __asm__ __volatile__(
        "mov %%cr3, %0"
        : "=r"(value)
    );

    return value;
}

void vmm_init(void) {
    uint64_t pml4_phys = read_cr3() & ~0xFFFULL;

    pml4 = (page_table_t *)phys_to_virt(pml4_phys);

}
uint16_t vmm_pml4_index(uint64_t virt) {
    return (virt >> 39) & 0b111111111;
}

uint16_t vmm_pdpt_index(uint64_t virt) {
    return (virt >> 30) & 0b111111111;
}

uint16_t vmm_pd_index(uint64_t virt) {
    return (virt >> 21) & 0b111111111;
}

uint16_t vmm_pt_index(uint64_t virt) {
    return (virt >> 12) & 0b111111111;
}

bool vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    if ((virt & 0xFFF) != 0 || (phys & 0xFFF) != 0) {
        return false;
    }

    uint16_t pml4_i = vmm_pml4_index(virt);
    uint16_t pdpt_i = vmm_pdpt_index(virt);
    uint16_t pd_i   = vmm_pd_index(virt);
    uint16_t pt_i   = vmm_pt_index(virt);

    // PML4 -> PDPT
    uint64_t pml4_entry = (*pml4)[pml4_i];

    if (!(pml4_entry & PAGE_PRESENT)) {
        uint64_t new_page_phys = pmm_alloc_page();
        if (!new_page_phys) {
            return false;
        }
        void *new_page_virt = phys_to_virt(new_page_phys);
        memset(new_page_virt, 0, 4096);
        (*pml4)[pml4_i] = new_page_phys | PAGE_PRESENT | PAGE_WRITABLE;
        pml4_entry = (*pml4)[pml4_i];
    }

    uint64_t pdpt_phys = pml4_entry & ~0xFFFULL;

    page_table_t *pdpt = (page_table_t *)phys_to_virt(pdpt_phys);


    // PDPT -> PD
    uint64_t pdpt_entry = (*pdpt)[pdpt_i];

    if (!(pdpt_entry & PAGE_PRESENT)) {
        uint64_t new_page_phys = pmm_alloc_page();
        if (!new_page_phys) {
            return false;
        }
        void *new_page_virt = phys_to_virt(new_page_phys);
        memset(new_page_virt, 0, 4096);
        (*pdpt)[pdpt_i] = new_page_phys | PAGE_PRESENT | PAGE_WRITABLE;
        pdpt_entry = (*pdpt)[pdpt_i];
    }

    uint64_t pd_phys = pdpt_entry & ~0xFFFULL;

    page_table_t *pd = (page_table_t *)phys_to_virt(pd_phys);


    // PD -> PT
    uint64_t pd_entry = (*pd)[pd_i];

    if (!(pd_entry & PAGE_PRESENT)) {
        uint64_t new_page_phys = pmm_alloc_page();
        if (!new_page_phys) {
            return false;
        }
        void *new_page_virt = phys_to_virt(new_page_phys);
        memset(new_page_virt, 0, 4096);
        (*pd)[pd_i] = new_page_phys | PAGE_PRESENT | PAGE_WRITABLE;
        pd_entry = (*pd)[pd_i];
    }

    uint64_t pt_phys = pd_entry & ~0xFFFULL;

    page_table_t *pt = (page_table_t *)phys_to_virt(pt_phys);

    (*pt)[pt_i] = phys | flags | PAGE_PRESENT;

    return true;
}