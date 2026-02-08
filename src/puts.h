#ifndef PUTS_H
#define PUTS_H

#include <stdint.h>

#define VGA_ADDRESS 0xB8000
#define VGA_ADDRESS_END 0xB8000 + 4000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void memset(void *dest, uint8_t content, unsigned long size);
void putc(uint8_t c, uint8_t color);
void puts(const char *s);
void puts_hex(uint64_t value);
void clear(void);

#endif
