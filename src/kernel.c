#include "idt.h"
#include "kalloc.h"
#include "kpanic.h"
#include "puts.h"
#include "vmm.h"
#include "schedule.h"
#include "task_handler.h"
#include <stdint.h>

extern char rodata_end[];
extern char text_start[];
extern char bss_end[];

void isr_handler(void) {
  puts("ISR Handler Called!\n");
  return;
}

void kmain(void *multiboot_info) {

  kmem_init((char *)PHYTOP, (char *)PHYEND);
  kvminit();
  init_idt();

  puts("meow\n");
  puts_hex(BASE((uint64_t)text_start));
  puts("\n");
  puts_hex(((uint64_t)rodata_end));
  puts("\n");
  puts_hex(BASE((uint64_t)bss_end));
  //__asm__ volatile("int $0");

  struct task *task_handler = InitTask((void *)handler, 0x1000);
  struct task *task_test = InitTask((void *)test, 0x1000);
  schedule();

  while (1) {
    hlt();
  }
}
