#include "idt.h"
#include "kalloc.h"
#include "kpanic.h"
#include "puts.h"
#include "spinlock.h"

extern uint64_t kernel_image_end;

void isr_handler(void) {
  puts("ISR Handler Called!\n");
  return;
}

void kmain(void *multiboot_info) {
  puts("Before interrupt\n");
  init_idt();
  kmem_init((char *)PHYTOP, (char *)PHYEND);

  puts("you r gay\n");

  __asm__ volatile("int $0");
  puts("im gay\n");

  clear();

  puts("hello again\n");

  kpanic("GAY");

  while (1) {
    hlt();
  }
}
