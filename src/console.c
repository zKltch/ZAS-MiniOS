#include "console.h"
#include "puts.h"
#include <stdint.h>

volatile ring_buffer_t ring_buffer = {0, 0, {}};

uint8_t buffer_getc(void) {
  uint8_t c;

  if (ring_buffer.head == ring_buffer.tail)
    return 0;

  c = ring_buffer.buffer[ring_buffer.tail & ring_buffer_boundary];
  ring_buffer.tail++;
  return c;
}

void buffer_readline(char *dst) {
  uint8_t c = 0;

  volatile line_buffer_t line_buffer = {0, {}};
  memset((void *)line_buffer.buffer, '\0', line_buffer_size);

  while (c != '\n' && line_buffer.len < line_buffer_size) {
    c = buffer_getc();

    if (c == backspace_key && line_buffer.len != 0) {
      backspace();
      line_buffer.buffer[--line_buffer.len] = '\0';
      continue;
    }

    if (c) {
      putc(c, 0xf);
      line_buffer.buffer[line_buffer.len++] = c;
    } else {
      sti();
      hlt();
    }
  }
  line_buffer.buffer[line_buffer.len] = '\0';
  memcpy(dst, (char *)line_buffer.buffer, line_buffer.len + 1);
}

void console_handler(void) {
  uint8_t c;
  char echo[0x100];

  for (;;) {

    puts("$ ");
    buffer_readline(echo);
    puts(echo);
    if (!memcmp(echo, "clear", 5)) {
      clear();
    }
  }
}
