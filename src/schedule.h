#ifndef SCHEDULE_H
#define SCHEDULE_H
#include <stdint.h>
#include "vmm.h"

#define max_pid 64
#define NULL (void *)0

struct task {
  uint64_t *kernel_rsp;
  uint64_t pid;
  pte_t cr3;
  enum { NEW, READY, RUNNING, WAITING, TERMINATED } state;
  void *stack;
  uint64_t stack_size;
  struct task *next_task;
};

extern uint64_t pid_bitmap;

uint64_t alloc_pid();

struct task *create_task(void (*entry)(), uint64_t stack_size);

void free_task_memory(struct task *task);

void destroy_task(struct task *task);


//use link list(temp)
extern struct task *schedule_list;
extern struct task *current;

void append_schedule_list(struct task *task);

extern void task_entrance(struct task *firstTask);

extern void switch_context(struct task *previous, struct task *next);

struct task *InitTask(void (*entry)(), uint64_t stack_size);

void schedule();
#endif
