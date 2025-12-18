global isr_stub

section .text
bits 64

extern isr_handler

isr_stub:
    push rbp
    mov rbp, rsp
    push rax
    push rcx
    push rdx
    push rbx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; call C handler (no args)
    call isr_handler

    ; restore
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbx
    pop rdx
    pop rcx
    pop rax
    pop rbp

    ; return from interrupt
    iretq
