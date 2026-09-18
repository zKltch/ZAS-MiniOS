#include "task_handler.h"
void handler() {
  puts("schedule!!!\n");
  schedule();
  while(1) { }
}

void test() {
  puts("\n");
  puts("test\n");
  while (1)
    ;
}
