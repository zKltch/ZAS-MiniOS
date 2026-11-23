#ifndef SYSCALL_H
#define SYSCALL_H
#include "trap.h"

#include <stdint.h>

#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2

uint64_t sys_read(regs *r);
uint64_t sys_write(regs *r);
uint64_t sys_open(regs *r);
#endif
