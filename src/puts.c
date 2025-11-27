#include "puts.h"
#include <stdint.h>

#define VGA_ADDRESS 0xB8000

void putc(uint8_t c, uint8_t color, uint16_t index) {
  uint16_t *vga_ptr = (uint16_t *)(0xB8000) + index;

  *vga_ptr = (color << 8) | c;
}

void puts(const char *s) {
  uint16_t string_index = 0;
  uint16_t vga_offset = 0;

  while (s[string_index] != '\0') {

    putc(s[string_index], 0xf, vga_offset);
    string_index++;
    vga_offset = vga_offset + 1;
  }
}
