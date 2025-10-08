#ifndef PUTS_H
#define PUTS_H

#include <stdint.h>

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void putc(uint8_t c,uint8_t color);
void puts(const char *s);
void puts_hex(uint64_t value);

#endif
