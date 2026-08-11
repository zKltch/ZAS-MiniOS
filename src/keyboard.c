#include "keyboard.h"
#include "console.h"
#include "pic.h"
#include <stdint.h>

static const char keymap[128] = {
    0,    27,  '1', '2',  '3',  '4', '5',  '6',  /* 0x00 */
    '7',  '8', '9', '0',  '-',  '=', '\b', '\t', /* 0x08 */
    'q',  'w', 'e', 'r',  't',  'y', 'u',  'i',  /* 0x10 */
    'o',  'p', '[', ']',  '\n', 0,   'a',  's',  /* 0x18 */
    'd',  'f', 'g', 'h',  'j',  'k', 'l',  ';',  /* 0x20 */
    '\'', '`', 0,   '\\', 'z',  'x', 'c',  'v',  /* 0x28 */
    'b',  'n', 'm', ',',  '.',  '/', 0,    '*',  /* 0x30 */
    0,    ' ', 0,   0,    0,    0,   0,    0,    /* 0x38 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 0x40 */
    0,    0,   '-', 0,    0,    0,   '+',  0,    /* 0x48 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 0x50 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 0x58 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 0x60 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 0x68 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 0x70 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 0x78 */
};

static const char keymap_shift[128] = {
    0,   27,  '!', '@', '#',  '$', '%',  '^',  /* 0x00 */
    '&', '*', '(', ')', '_',  '+', '\b', '\t', /* 0x08 */
    'Q', 'W', 'E', 'R', 'T',  'Y', 'U',  'I',  /* 0x10 */
    'O', 'P', '{', '}', '\n', 0,   'A',  'S',  /* 0x18 */
    'D', 'F', 'G', 'H', 'J',  'K', 'L',  ':',  /* 0x20 */
    '"', '~', 0,   '|', 'Z',  'X', 'C',  'V',  /* 0x28 */
    'B', 'N', 'M', '<', '>',  '?', 0,    '*',  /* 0x30 */
    0,   ' ', 0,   0,   0,    0,   0,    0,    /* 0x38 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 0x40 */
    0,   0,   '-', 0,   0,    0,   '+',  0,    /* 0x48 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 0x50 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 0x58 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 0x60 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 0x68 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 0x70 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 0x78 */
};

#define left_shift 0x2A
#define right_shift 0x36
#define press 0x7f
#define release 0x80

#define keymap_index(scancode) (scancode & press)
volatile uint8_t shift = 0;
extern ring_buffer_t ring_buffer;

void keyboard_handler() {
  uint8_t scancode = inb(0x60);

  if ((scancode & ~release) == left_shift ||
      (scancode & ~release) == right_shift) {
    shift = !(scancode & release);
    goto ret;
  }

  if (scancode & release)
    goto ret;

  if (shift) {

    ring_buffer.buffer[ring_buffer.head & ring_buffer_boundary] =
        keymap_shift[keymap_index(scancode)];
    ring_buffer.head++;
    goto ret;
  }

  ring_buffer.buffer[ring_buffer.head & ring_buffer_boundary] =
      keymap[keymap_index(scancode)];
  ring_buffer.head++;
ret:
  PIC_sendEOI((uint8_t)1);
}
