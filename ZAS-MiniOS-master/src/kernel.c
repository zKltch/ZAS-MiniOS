#include "puts.h"

void kmain(void *multiboot_info)
{
    puts("x\n");
    puts("abc\n");
    //puts_hex((uint64_t)multiboot_info);	//it will break down

    // stop the kernel
    while (1) {
	__asm__("jmp .");
    }
}

