#ifndef VMM_H
#define VMM_H
#include <stdint.h>

typedef uintptr_t pte_t;
#define PTE_P 1 // present
#define PTE_W 2 // write
#define PTE_U 4 // usermode
// pm_level 4 3 2 1
// 39 30 21 12
//  & 0x1ff (9 bits)
#define PX(PM_level, va) ((va >> 3) >> (9 * PM_level)) & 0x1ff
// physical address to page table entry
// bit shifting 12 do clear flag bits and aligned with page table size(4k)
#define PA2PTE(PA) (pte_t *)(PA & ~0xFFF)
#define PTE2PA(PTE) (pte_t)(PTE)

// kbase and virtual physical memory convert
#define KERNELBASE 0xFFFFFFFF80000000
#define v2p(va) ((uint64_t)va - KERNELBASE)
#define p2v(va) (KERNELBASE + (uint64_t)va)
#define BASE(m) m & ~0xFFF

// functions
void switch_cr3(uintptr_t *pdbr);
pte_t *walk(pte_t *pml4, uint64_t va, int alloc);
int map_page(pte_t *pml4, uint64_t va, uint64_t pa_begin, uint64_t pa_end,
             int perm);
void kvminit();

static inline void lcr3(uintptr_t *val) {
  asm volatile("mov %0,%%cr3" : : "r"(val));
}

uint64_t create_process_pml4(void);

void free_process_pml4(pte_t cr3);

#endif
