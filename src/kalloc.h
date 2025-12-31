#ifndef KALLOC_H
#define KALLOC_H
#include <stdint.h>

#define PGSIZE 0x0000000000001000 // 4096

// hard coded init start and end address
#define PHYTOP 0x0000000000120000
#define PHYEND 0x0000000000190000

void kmem_init(char *begin, char *end);
char *kalloc(void);
void kfree(char *p);

#endif
