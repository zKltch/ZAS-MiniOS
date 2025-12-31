#include "kalloc.h"
#include "kpanic.h"
#include "spinlock.h"
#include <stdint.h>

struct free_node {
  struct free_node *next;
};

struct {
  struct free_node *freelist;
} kmem;

// from kmem end to begin init
void kmem_init(char *begin, char *end) {
  char *p = end;
  for (; begin <= p; p = p - PGSIZE) {
    kfree(p);
  }
}

void kfree(char *p) {
  struct free_node *run;

  if ((uintptr_t)p % PGSIZE != 0 || (uintptr_t)p > PHYEND ||
      (uintptr_t)p < PHYTOP) {
    kpanic("kfree");
  }

  run = (struct free_node *)p;

  run->next = kmem.freelist;
  kmem.freelist = run;
}

char *kalloc(void) {

  struct free_node *alloc_chunk;

  alloc_chunk = kmem.freelist;
  kmem.freelist = alloc_chunk->next;

  return (char *)alloc_chunk;
}
