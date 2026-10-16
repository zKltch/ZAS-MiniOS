#ifndef VMM_H
#define VMM_H
#include <stdint.h>

typedef uintptr_t pte_t;
#define PTE_P 1UL          // present
#define PTE_W 2UL          // write
#define PTE_U 4UL          // usermode
#define PTE_NX (1UL << 63) // NX
// pm_level 4 3 2 1
// 39 30 21 12
//  & 0x1ff (9 bits)
#define PX(PM_level, va) (((((uint64_t)(va)) >> 3) >> (9 * (PM_level))) & 0x1ff)
// physical address to page table entry
// bit shifting 12 do clear flag bits and aligned with page table size(4k)
#define PA2PTE(pa) (((uint64_t)(pa)) & ~0xFFFUL)
#define PTE2PA(pte) (((uint64_t)(pte)) & ~0xFFFUL)
#define PTE2VA(pte) ((pte_t *)p2v(PTE2PA(pte)))

// kbase and virtual physical memory convert
#define KERNELBASE 0xFFFFFFFF80000000
#define v2p(va) ((uint64_t)(va) - KERNELBASE)
#define p2v(pa) (KERNELBASE + (uint64_t)(pa))
#define BASE(m) ((m) & ~0xFFFUL)

// functions
void switch_cr3(uintptr_t *pdbr);
pte_t *walk(pte_t *pml4, uint64_t va, int alloc);
int map_page(pte_t *pml4, uint64_t va, uint64_t pa_begin, uint64_t pa_end,
             uint64_t perm);
void kvminit();

static inline void lcr3(uintptr_t *val) {
  asm volatile("mov %0,%%cr3" : : "r"(val));
}

uint64_t create_process_pml4(void);

void free_process_pml4(pte_t cr3);

#endif
