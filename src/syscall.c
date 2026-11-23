#include "syscall.h"
#include "puts.h"
#include "trap.h"
#include <stdint.h>

uint64_t sys_read(regs *r);
uint64_t sys_write(regs *r);
uint64_t sys_open(regs *r);

uint64_t (*syscall_table[])(regs *r) = {
    [SYS_READ] = sys_read, [SYS_WRITE] = sys_write, [SYS_OPEN] = sys_open};

#define sys_num_max (sizeof(syscall_table) / sizeof(syscall_table[0]))

int do_syscall(regs *r) {
  if (r->rax < 0 || r->rax >= sys_num_max || syscall_table[r->rax] == 0ul) {
    puts("[*] SYSCALL - Unknown syscall number ");
    puts_hex(r->rax);
    puts("\n");
    r->rax = -1;
    return -1;
  }
  syscall_table[r->rax](r);
  return 1;
}

// syscall functions
uint64_t sys_read(regs *r) { return 0; }
uint64_t sys_write(regs *r) {
  puts("test");
  return 1;
}
uint64_t sys_open(regs *r) { return 0; }
