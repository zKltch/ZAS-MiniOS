global irq0_stub

section .text
bits 64

irq0_stub:
    ; harmless stub for unused IRQ: just return from interrupt
    iretq
