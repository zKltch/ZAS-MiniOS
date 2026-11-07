#include "console.h"
#include "script_engine.h"
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
  char cmd[0x100];

  for (;;) {

    puts("$"); puts(path); puts("> ");
    buffer_readline(cmd);
    execute_command(cmd);
  }
}

void execute_command(char *cmd)
{
  if (!memcmp(cmd, "clear", 5)) clear();
  if (!memcmp(cmd, "cat", 3)) cat_file(cmd + 3);
  if (!memcmp(cmd, "echo", 4)) echo_write_file(cmd + 4);
  if (!memcmp(cmd, "sh", 2)) cmd_script(cmd + 2);
  if (!memcmp(cmd, "mkdir", 5)) create_dir(cmd + 5);
  if (!memcmp(cmd, "cd", 2)) change_directory(cmd + 2);
  if (!memcmp(cmd, "ls", 2)) cmd_ls();
}
