#include "console.h"
#include "script_engine.h"
#include "puts.h"
#include <stdint.h>

volatile ring_buffer_t ring_buffer = {0, 0, {}};

static uint16_t start_cursor = 0;
// static int offset_cursor = 0;
static uint16_t cursor_index = 0;

char path[0x100] = { 0 };
char *path_ptr = path;
uint8_t path_array[0x100] = { 0 };
uint8_t path_step = 0;
uint32_t CurrentID = 0; // it should be saved in task

// called after inserting or deleting char
static void buffer_refreshline(volatile line_buffer_t *line_buffer)
{
  volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;

  for(uint16_t i = 0; i < line_buffer->len; i++)
  {
    uint16_t refresh_position = (start_cursor + i) << 1;
    vga_ptr[refresh_position] = line_buffer->buffer[i];
    vga_ptr[refresh_position + 1] = 0x0f;
  }

  uint16_t clear_position = (start_cursor + line_buffer->len) << 1;
  vga_ptr[clear_position] = ' ';
  vga_ptr[clear_position + 1] = 0x07;

  StringAddress = (start_cursor + cursor_index) << 1;
  update_cursor(0);
}

static void buffer_shl(volatile line_buffer_t *line_buffer)
{
  for(uint16_t i = cursor_index - 1; i < line_buffer->len - 1; i++)
  {
    line_buffer->buffer[i] = line_buffer->buffer[i + 1];
  }
}

static void buffer_shr(volatile line_buffer_t *line_buffer)
{
  for(uint16_t i = line_buffer->len; i > cursor_index; i--)
  {
    line_buffer->buffer[i] = line_buffer->buffer[i - 1];
  }
}

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

  // line_buffer length must less than size - 1, avoid os crashing
  while (c != '\n' && line_buffer.len < line_buffer_size - 1) {
    c = buffer_getc();

    switch (c)
    {
      case backspace_key :
        if (cursor_index == 0) continue;
        buffer_shl(&line_buffer);
        cursor_index--;
        // backspace();
        line_buffer.buffer[--line_buffer.len] = '\0';
        buffer_refreshline(&line_buffer);
	continue;
      case escape_key :
      case up_key :
      case down_key :
	continue;
      case left_key :
        if (cursor_index > 0)
        {
          cursor_index--;
          update_cursor(-1);
        }
	continue;
      case right_key :
        if (cursor_index < line_buffer.len)
        {
          cursor_index++;     
          update_cursor(1);
        }
        continue;
    }

    if (c) {
      if (cursor_index != line_buffer.len) buffer_shr(&line_buffer);
      line_buffer.buffer[cursor_index++] = c;
      line_buffer.len++;
      if (cursor_index == line_buffer.len) putc(c, 0xf);
      else buffer_refreshline(&line_buffer);
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
    start_cursor = (StringAddress >> 1);
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
  if (!memcmp(cmd, "rm", 2)) remove_file(cmd + 2);
}
