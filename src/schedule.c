#include "schedule.h"
#include "kalloc.h"
#include "puts.h"
#include "idt.h"

uint64_t pid_bitmap = 0xFFFF;
extern uintptr_t *KERNEL_PDBR;
struct task *schedule_list = NULL;

struct task *current = NULL;

uint64_t FirstEnterTask = 1;

uint64_t alloc_pid() {
  for (uint64_t pid = 1; pid <= max_pid; pid++) {
    if (pid_bitmap & (1ULL << pid)) {
      pid_bitmap ^= (1ULL << pid);
      return pid;
    }
  }
  return 0;
}

struct task *create_task(void (*entry)(), uint64_t stack_size) {
  struct task *new_task = (struct task *)kalloc();
  memset(new_task, 0, sizeof(*new_task));

  new_task->stack = kalloc();
  uint64_t temp = ((stack_size % PGSIZE) ? ((stack_size / PGSIZE) + 1)
                                         : (stack_size / PGSIZE));
  for (int i = 1; i < temp; i++) {
    kalloc();
  }

  new_task->stack_size = temp * PGSIZE;

  memset(new_task->stack, 0, new_task->stack_size);

  uint64_t *stack =
      (uint64_t *)((uint8_t *)new_task->stack + new_task->stack_size);
  
  uint64_t *stack_top = stack;
  *(--stack) = 0x10;                     //SS(user space need to change)
  *(--stack) = (uint64_t)stack_top;      //RSP
  *(--stack) = 0x202;                    //RFLAGS(IF | bit1_reserved)
  *(--stack) = 0x8;                      //CS(user space need to change)
  *(--stack) = (uint64_t)entry;          //RIP(IDT)
  // follow stub.s
  *(--stack) = 0;                        // RAX
  *(--stack) = 0;                        // RCX
  *(--stack) = 0;                        // RDX
  *(--stack) = 0;                        // RSI
  *(--stack) = 0;                        // RDI
  *(--stack) = 0;                        // R8
  *(--stack) = 0;                        // R9
  *(--stack) = 0;                        // R10
  *(--stack) = 0;                        // R11
  // follow switch.s
  //*(--stack) = (uint64_t)entry;          // RIP(switch)
  *(--stack) = 0;                        // RBX
  *(--stack) = 0;                        // RBP
  *(--stack) = 0;                        // R12
  *(--stack) = 0;                        // R13
  *(--stack) = 0;                        // R14
  *(--stack) = 0;                        // R15

  new_task->kernel_rsp = stack;
  // new_task->cr3 = create_process_pml4(); uncomment after start making
  // userspace
  new_task->cr3 = (pte_t)KERNEL_PDBR; // KERNEL space only

  new_task->state = NEW;

  return new_task;
}

void free_task_memory(struct task *task) {
  char *stack = (char *)((uint8_t *)task->stack + task->stack_size);

  for (int i = 0; i < task->stack_size / PGSIZE; i++) {
    stack -= PGSIZE;
    kfree(stack);
  }
  return;
}

void destroy_task(struct task *task) {
  free_process_pml4(task->cr3);

  free_task_memory(task);

  kfree((char *)task);
  return;
}

void append_schedule_list(struct task *task) {
  task->state = READY;
  if (!schedule_list) {
    schedule_list = task;
    current = schedule_list;
    task->next_task = task;
  } else {
    task->next_task = schedule_list->next_task;
    schedule_list->next_task = task;
    schedule_list = task;
  }
  return;
}

struct task *InitTask(void (*entry)(), uint64_t stack_size) {
  struct task *NewTask = create_task(entry, stack_size);

  NewTask->pid = alloc_pid();
  if (NewTask->pid == 0) {
    destroy_task(NewTask);
    return NULL;
  }

  append_schedule_list(NewTask);
  return NewTask;
}

void __schedule(char IsTimer) {
  if (schedule_list == NULL)
    return;
  if (!IsTimer)
    cli(); 
  if (FirstEnterTask) {
    current->state = RUNNING;
    if(!IsTimer) {
      FirstEnterTask = 0;
      task_entrance(current);
    }
  } else {
    /*if (current == current->next_task)
      return;*/    //To make schedule simple

    current->state = READY;
    struct task *prev = current;

    current = current->next_task;

    current->state = RUNNING;
    struct task *next = current;

    if(!IsTimer){
      switch_context(prev, next);
    }
  }
  return;
}
