#include "kpanic.h"

void kpanic(char *s) {
  clear();
  puts("[*] KERNEL PANIC - ");
  puts(s);

  while (1)
    hlt();
}
