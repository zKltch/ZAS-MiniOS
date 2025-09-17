#include "puts.h"

void kmain(void *multiboot_info) {

    puts("Auron is gay");

    // stop the kernel
    while (1) {
    __asm__("jmp ."); // Jump to the current instruction
    }
}

