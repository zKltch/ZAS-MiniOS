#include "serial.h"
#include <stdint.h>
// it can be provide for debug!!
/* I/O port helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void serial_init(void) {
    /* COM1: 0x3F8 */
    outb(0x3F8 + 1, 0x00);    // Disable all interrupts
    outb(0x3F8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(0x3F8 + 0, 0x03);    // Divisor LSB (38400 baud)
    outb(0x3F8 + 1, 0x00);    // Divisor MSB
    outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static int serial_is_transmit_empty(void) {
    return inb(0x3F8 + 5) & 0x20;
}

void serial_putc(char c) {
    while (!serial_is_transmit_empty()) { /* spin */ }
    outb(0x3F8, (uint8_t)c);
}

void serial_puts(const char *s) {
    for (const char *p = s; *p; ++p) {
        if (*p == '\n') serial_putc('\r'); /* CR+LF */
        serial_putc(*p);
    }
}
