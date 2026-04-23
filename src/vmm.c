#include "vmm.h"
#include "kalloc.h"
#include "kpanic.h"
#include "puts.h"
#include <stdint.h>

extern void pa_to_highhalfkernel(void);
extern char text_start[];
extern char rodata_end[];
extern char data_start[];
extern char bss_end[];
uintptr_t *KERNEL_PDBR;

struct kmap {
  uint64_t va_base;
  uint64_t pa_begin;
  uint64_t pa_end;
  uint32_t perm;
} kmap[] = {
    {KERNELBASE, 0, (uint64_t)text_start, PTE_W}, // IO
    {p2v(text_start), (uint64_t)text_start, (uint64_t)(rodata_end),
     0}, // .text and .rodata
    {p2v((data_start)), (uint64_t)(data_start), (uint64_t)(bss_end),
     PTE_W}, // data and bss
    {0, 0, (uint64_t)(256 * 1024 * 1024),
     PTE_W}}; // direct mapping so it wont page fault after switching cr3

void switch_cr3(uintptr_t *pdbr);

// PML4 > PDPT > PD > PT
// return address of page table entry
pte_t *walk(pte_t *pml4, uint64_t va, int alloc) {
  pte_t *pte = pml4;
  pte_t *ptaloc;

  for (int page_level = 4; page_level > 1; page_level--) {
    pte = &(pte[PX(page_level, va)]); // address of current page_level entry

    if (*pte & PTE_P) {
      pte = PA2PTE(*pte);
      continue;
    }

    if (!alloc || !(ptaloc = (pte_t *)kalloc()))
      return 0;

    memset(ptaloc, 0, PGSIZE);
    *pte = PTE2PA(ptaloc) | PTE_P | PTE_W | PTE_U;
    pte = PA2PTE(*pte);
  }
  return &(pte[PX(1, va)]);
}

int map_page(pte_t *pml4, uint64_t va, uint64_t pa_begin, uint64_t pa_end,
             int perm) {
  uint64_t c;
  pte_t *pte;

  if ((va % PGSIZE != 0 || pa_begin % PGSIZE != 0 || pa_end % PGSIZE != 0))
    kpanic("page mapping memory not aligned with page size");

  for (c = pa_begin; c < pa_end; c += PGSIZE) {
    pte = walk(pml4, va, 1);

    if (!pte)
      return 0;

    if (*pte & PTE_P)
      kpanic("remapping");

    *pte = c | perm | PTE_P;
    va += PGSIZE;
  }
  return 1;
}

void kvminit() {

  KERNEL_PDBR = (uintptr_t *)kalloc();
  memset(KERNEL_PDBR, 0, PGSIZE);

  for (int i = 0; i < sizeof(kmap) / sizeof(struct kmap); i++) {
    map_page(KERNEL_PDBR, BASE(kmap[i].va_base), BASE(kmap[i].pa_begin),
             BASE(kmap[i].pa_end), kmap[i].perm);
  }

  switch_cr3(KERNEL_PDBR);
  pa_to_highhalfkernel();
}

void switch_cr3(uintptr_t *pdbr) { lcr3(pdbr); }

uint64_t create_process_pml4() {
  uintptr_t *process_pml4 = (uintptr_t *)kalloc();
  memset(process_pml4, 0, PGSIZE);

  for (int i = 0; i < sizeof(kmap) / sizeof(struct kmap); i++) {
    process_pml4[i] = KERNEL_PDBR[i];
  }

  return (uint64_t)process_pml4;
}

void free_process_pml4(pte_t cr3) {
  kfree((char *)cr3);
}
