#include "spinlock.h"

uint32_t cli_depth = 0;

void acquire() {
  __asm__ volatile("cli");
  cli_depth++;
}

void release() {
  cli_depth--;
  if (cli_depth == 0)
    __asm__ volatile("sti");
}
