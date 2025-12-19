#include "idt.h"
#include "puts.h"
#include <stdint.h>

void isr_handler(void) {
  puts("ISR Handler Called!\n");
  return;
}

void kmain(void *multiboot_info) {
  puts("Before interrupt\n");
  init_idt();

  puts("you r gay\n");

  __asm__ volatile("int $0");
  puts("im gay\n");

  while (1) {
    __asm__("hlt");
  }
}
