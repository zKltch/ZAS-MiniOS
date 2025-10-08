#include "puts.h"

uint16_t StringAddress = 0;

void putc(uint8_t c,uint8_t color)
{
    volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;
    volatile uint16_t *StringAddressPtr = &StringAddress;
    if(c == '\n'){
	*StringAddressPtr = (*StringAddressPtr / (VGA_WIDTH * 2) + 1) * (VGA_WIDTH * 2);
    }else{
	vga_ptr = vga_ptr + *StringAddressPtr + 2;
    	*vga_ptr=c;
    	vga_ptr++;
	*vga_ptr=color;
	*StringAddressPtr = *StringAddressPtr + 2;
    }
} 

void puts(const char *s)
{
    while (*s != '\0')
    {
	putc(*s, 0xf);
        s++;
    }

}

void puts_hex(uint64_t value)
{
    char hex[17] = "0123456789ABCDEF";
    char buffer[17];
    char *ptr = &buffer[16];

    *ptr = '\0';

    puts("0x");

    if(value == 0){
	putc('0', 0xf);
	return;
    }

    while (value != 0){
	ptr--;
	*ptr = hex[value % 16];
	value /= 16;
    }

    while(*ptr != 0){
    	putc(*ptr, 0xf);
	ptr++;
    }
}
