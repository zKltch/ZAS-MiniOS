#include "puts.h"
#include "idt.h"

void isr_handler(void) {
  puts("ISR Handler Called!\n");
  return;
}

void kmain(void *multiboot_info) {
  puts("Before interrupt\n");
  init_idt();
  
  puts("you r gay");
  /* 觸發軟體中斷 0 — handler 會執行並印出訊息 */
  __asm__ volatile ("int $0");
  puts("im gay");
  
  while (1) {
    __asm__("hlt");
  }
}