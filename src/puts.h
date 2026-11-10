#ifndef PUTS_H
#define PUTS_H

#include "vmm.h"
#include <stdint.h>

extern uint16_t StringAddress;

#define VGA_ADDRESS_SIZE 0x8000
#define VGA_ADDRESS p2v(0xB8000)
#define VGA_ADDRESS_END (VGA_ADDRESS + VGA_ADDRESS_SIZE)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_PAGE_SIZE (VGA_WIDTH * VGA_HEIGHT* 2)
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

typedef union _WORD_INTEGER
{
  struct
  {
    uint8_t LowPart;
    uint8_t HighPart;
  };
  uint16_t WordPart;
}WORD_INTEGER, *PWORD_INTEGER;

void update_cursor(short Move);
void putc(uint8_t c, uint8_t color);
void puts(const char *s);
void puts_hex(uint64_t value);
void clear(void);
void backspace(void);
void scroll(void);
void memset(void *dest, uint8_t content, unsigned long size);
void memcpy(void *dest, void *src, unsigned long size);
int memcmp(void *dest, void *src, unsigned long size);

#endif
