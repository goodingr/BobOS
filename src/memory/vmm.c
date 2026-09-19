#include "vmm.h"
#include "memory.h"
#include "serial.h"
#include "string.h"

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER     (1ULL << 2)

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

    uint64_t addr = (uint64_t)&vmm_init;

    uint16_t pml4_i = vmm_pml4_index(addr);
    uint16_t pdpt_i = vmm_pdpt_index(addr);
    uint16_t pd_i   = vmm_pd_index(addr);
    uint16_t pt_i   = vmm_pt_index(addr);

    // PML4 -> PDPT
    uint64_t pml4_entry = (*pml4)[pml4_i];

    if (!(pml4_entry & PAGE_PRESENT)) {
        return;
    }

    uint64_t pdpt_phys = pml4_entry & ~0xFFFULL;

    page_table_t *pdpt = (page_table_t *)phys_to_virt(pdpt_phys);


    // PDPT -> PD
    uint64_t pdpt_entry = (*pdpt)[pdpt_i];

    if (!(pdpt_entry & PAGE_PRESENT)) {
        return;
    }

    uint64_t pd_phys = pdpt_entry & ~0xFFFULL;

    page_table_t *pd = (page_table_t *)phys_to_virt(pd_phys);


    // PD -> PT
    uint64_t pd_entry = (*pd)[pd_i];

    if (!(pd_entry & PAGE_PRESENT)) {
        return;
    }

    uint64_t pt_phys = pd_entry & ~0xFFFULL;

    page_table_t *pt = (page_table_t *)phys_to_virt(pt_phys);


    // PT -> physical page
    uint64_t pt_entry = (*pt)[pt_i];

    if (!(pt_entry & PAGE_PRESENT)) {
        return;
    }

    uint64_t page_phys = pt_entry & ~0xFFFULL;


    // Offset within the 4096-byte page
    uint64_t offset = addr & 0xFFFULL;

    uint64_t final_phys = page_phys + offset;

    char str[21];
    // final_phys is the physical address corresponding to &vmm_init
    serial_write("\nVMINIT Virtual Address: ");
    uint64_to_hex(addr, str);
    serial_write(str);
    serial_write("\nVMINIT Physical Address: ");
    uint64_to_hex(final_phys, str);
    serial_write(str);
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