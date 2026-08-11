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

#endif
