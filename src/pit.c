#include"pit.h"

void pit_init(uint32_t frequency)
{
    uint16_t divisor = PIT_FREQUENCY / frequency;

    // Select Channel : Channel 0		00
    // Access Mode : low byte/high byte		11
    // Operation Mode : Mode 3 -> square wave	011
    // BCD/Binary Mode : Binary Mode		0
    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0, divisor & 0xFF);

    outb(PIT_CHANNEL0, divisor >> 8);
}

volatile uint64_t timer_test = 0;

void PitTimer_handler(void)
{
    timer_test++;

    PIC_sendEOI((uint8_t)0);

    schedule_timer();
}
