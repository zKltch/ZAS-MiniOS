#ifndef CNS_H
#define CNS_H
#include "idt.h"
#include "pic.h"
#include "puts.h"
#include <stdint.h>

#define ring_buffer_size 0x100
#define ring_buffer_boundary ring_buffer_size - 1
#define line_buffer_size ring_buffer_size

#define backspace_key 0x8
#define escape_key 0x1B
#define up_key 0x81
#define down_key 0x82
#define left_key 0x83
#define right_key 0x84

extern char path[0x100];
extern char *path_ptr;
extern uint8_t path_array[0x100];
extern uint8_t path_step;
extern uint32_t CurrentID;

typedef struct {
  volatile uint16_t head;
  volatile uint16_t tail;
  uint8_t buffer[ring_buffer_size];
} ring_buffer_t;

typedef struct {
  volatile uint16_t len;
  uint8_t buffer[line_buffer_size];
} line_buffer_t;

uint8_t buffer_getc(void);
void buffer_readline(char *dst);
void console_handler(void);
void execute_command(char *cmd);

#endif
