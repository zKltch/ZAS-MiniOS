#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

typedef struct {
  uint64_t r15, r14, r13, r12, rbp, rbx;              // callee-saved
  uint64_t r11, r10, r9, r8, rdi, rsi, rdx, rcx, rax; // caller-saved
  uint64_t rip, cs, rflags, rsp, ss;
} regs;

#endif
