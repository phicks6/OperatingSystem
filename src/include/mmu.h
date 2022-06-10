#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct PageTable {
    uint64_t entries[512];
} PageTable;

#define PB_NONE     0
#define PB_VALID    (1UL << 0)
#define PB_READ     (1UL << 1)
#define PB_WRITE    (1UL << 2)
#define PB_EXECUTE  (1UL << 3)
#define PB_USER     (1UL << 4)
#define PB_GLOBAL   (1UL << 5)
#define PB_ACCESS   (1UL << 6)
#define PB_DIRTY    (1UL << 7)

#define SATP_MODE_BIT    60
#define SATP_MODE_SV39   (8UL << SATP_MODE_BIT)
#define SATP_ASID_BIT    44
#define SATP_PPN_BIT     0
#define SATP_SET_PPN(x)  ((((uint64_t)x) >> 12) & 0xFFFFFFFFFFFUL)
#define SATP_SET_ASID(x) ((((uint64_t)x) & 0xFFFF) << 44)

bool mmu_map(PageTable *tab, uint64_t vaddr, uint64_t paddr, uint64_t bits);
void mmu_free(PageTable *tab);
uint64_t mmu_translate(PageTable *tab, uint64_t vaddr);
void mmu_map_multiple(PageTable *tab, uint64_t start_virt,
                      uint64_t start_phys, uint64_t num_bytes,
                      uint64_t bits);

#define KERNEL_ASID 0xFFFFUL
extern PageTable *kernel_mmu_table;


#define sfence() \
    asm volatile("sfence.vma zero, zero");
#define sfence_asid1(asid)              \
    asm volatile("sfence.vma zero, %0" \
                 :                     \
                 : "r"(asid));
#define sfence_asid2(asid, vaddr)       \
    asm volatile("sfence.vma %0, %1"   \
                 :                     \
                 : "r"(vaddr), "r"(asid));