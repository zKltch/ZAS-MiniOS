#include "puts.h"
#include <stdint.h>

uint16_t StringAddress = 0;
// static uint16_t StringAddress = 0;

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// This code is specifically designed to work around a broken compiler
// The compiler bug has been fixed in the new toolchain
/*
void putc(uint8_t c, uint8_t color) {
  volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;
  volatile uint16_t *StringAddressPtr = &StringAddress;
  if (c == '\n') {
    *StringAddressPtr =
        (*StringAddressPtr / (VGA_WIDTH * 2) + 1) * (VGA_WIDTH * 2);
  } else {
    vga_ptr = vga_ptr + *StringAddressPtr + 2;
    *vga_ptr = c;
    vga_ptr++;
    *vga_ptr = color;
    *StringAddressPtr = *StringAddressPtr + 2;
  }
}

void backspace() {

volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;
volatile uint16_t *StringAddressPtr = &StringAddress;

vga_ptr = vga_ptr + *StringAddressPtr;
*vga_ptr = ' ';
vga_ptr++;
*vga_ptr = 0x07;
*StringAddressPtr = *StringAddressPtr - 2;
}
*/

void update_cursor(short Move)
{
  StringAddress += (Move << 1);
  if (StringAddress < 0) StringAddress = 0;
  if (StringAddress + (VGA_WIDTH * 2) >= VGA_ADDRESS_SIZE)
  {
    volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;
    uint16_t vga_page_start = StringAddress - (VGA_HEIGHT - 1) * (VGA_WIDTH * 2);
    for(uint16_t i = 0; i < VGA_PAGE_SIZE; i++) vga_ptr[vga_page_start + i];
    StringAddress = (VGA_HEIGHT - 1) * (VGA_WIDTH * 2);
  }

  uint16_t current_line = StringAddress / (VGA_WIDTH * 2);
  WORD_INTEGER vga_start_offset;
  vga_start_offset.WordPart = 0;
  if (current_line >= VGA_HEIGHT) vga_start_offset.WordPart = (current_line - (VGA_HEIGHT - 1)) * VGA_WIDTH;

  outb(VGA_CTRL_PORT, 0xD); // LOW
  outb(VGA_DATA_PORT, vga_start_offset.LowPart);

  outb(VGA_CTRL_PORT, 0xC); // HIGH
  outb(VGA_DATA_PORT, vga_start_offset.HighPart);

  WORD_INTEGER Cursor;
  Cursor.WordPart = StringAddress >> 1;

  outb(VGA_CTRL_PORT, 0xF); // LOW
  outb(VGA_DATA_PORT, Cursor.LowPart);

  outb(VGA_CTRL_PORT, 0xE); // HIGH
  outb(VGA_DATA_PORT, Cursor.HighPart);
}

void putc(uint8_t c, uint8_t color) {
  volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;
  if (c == '\n') {
    StringAddress = (StringAddress / (VGA_WIDTH << 1) + 1) * (VGA_WIDTH << 1);
  } else {
    vga_ptr[StringAddress] = c;
    vga_ptr[StringAddress + 1] = color;
    StringAddress = StringAddress + 2;
  }
  update_cursor(0);
}

void backspace() {
  volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;

  StringAddress -= 2;
  vga_ptr[StringAddress] = ' ';
  vga_ptr[StringAddress + 1] = 0x07;

  update_cursor(0);
}

void puts(const char *s) {
  while (*s != '\0') {
    putc(*s, 0xf);
    s++;
  }
}

void puts_hex(uint64_t value) {
  char hex[17] = "0123456789ABCDEF";
  char buffer[17];
  char *ptr = &buffer[16];

  *ptr = '\0';

  puts("0x");

  if (value == 0) {
    putc('0', 0xf);
    return;
  }

  while (value != 0) {
    ptr--;
    *ptr = hex[value & 0xf];
    value >>= 4;
  }

  while (*ptr != 0) {
    putc(*ptr, 0xf);
    ptr++;
  }
}

void clear(void) {
  uint8_t *vga_begin = (uint8_t *)VGA_ADDRESS;

  for (; vga_begin <= (uint8_t *)VGA_ADDRESS_END; vga_begin += 2)
  {
    vga_begin[0] = ' ';
    vga_begin[1] = 0x07;
  }

  StringAddress = 0;

  update_cursor(0);
}

void scroll(void)
{

}

void memset(void *dest, uint8_t content, unsigned long size) {
  uint8_t *set = (uint8_t *)dest;

  for (unsigned long i = 0; i < size; i++)
    set[i] = content;
}

void memcpy(void *dest, void *src, unsigned long size) {
  uint8_t *set = (uint8_t *)dest;
  uint8_t *src_set = (uint8_t *)src;

  for (unsigned long i = 0; i < size; i++)
    set[i] = src_set[i];
}

int memcmp(void *dest, void *src, unsigned long size) {
  uint8_t *set = (uint8_t *)dest;
  uint8_t *src_set = (uint8_t *)src;

  for (unsigned long i = 0; i < size; i++)
    if (set[i] != src_set[i]) {
      return 1;
    }
  return 0;
}
