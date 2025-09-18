#include "puts.h"

void kmain(void *multiboot_info) {

    //puts("p23 is gay");
    puts("abc");

    // stop the kernel
    while (1) {
    __asm__("jmp .");
    }
}

