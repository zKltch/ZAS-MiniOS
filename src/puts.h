#ifndef PUTS_H
#define PUTS_H

#include "vmm.h"
#include <stdint.h>

#define VGA_ADDRESS p2v(0xB8000)
#define VGA_ADDRESS_END (VGA_ADDRESS + 4000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void memset(void *dest, uint8_t content, unsigned long size);
void putc(uint8_t c, uint8_t color);
void puts(const char *s);
void puts_hex(uint64_t value);
void clear(void);
void backspace();
void memcpy(void *dest, void *src, unsigned long size);
int memcmp(void *dest, void *src, unsigned long size);

#endif
