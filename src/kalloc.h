#ifndef KALLOC_H
#define KALLOC_H
#include <stdint.h>

extern char bss_end[];

#define PGSIZE 0x0000000000001000 // 4096

// init start and end address
// memory layout
// low address
// ----------- mmio and bootloader
// ----------- kernel text data rodata bss
// ----------- unused ram
// ----------- unused ram
// high address
// PHYTOP begin after bss
#define PHYTOP (uint64_t)bss_end  // low address
#define PHYEND 0x0000000010000000 // high address 256M

void kmem_init(char *begin, char *end);
char *kalloc(void);
void kfree(char *p);

#endif
