#include "puts.h"

void kmain(void *multiboot_info) {

  // puts("p23 is gay");
  puts("nigga");

  // putc('a', 0x0f, 0);
  //  stop the kernel
  while (1) {
    __asm__("jmp .");
  }
}
