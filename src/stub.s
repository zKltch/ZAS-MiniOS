bits 64
section .text


%macro STUB 2                   ; 
global %1
extern %2
%1:
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    cld
    call %2
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    iretq
%endmacro

; stub name and their handler
STUB isr_stub, isr_handler      ; stub name, C handler
STUB keyboard_stub, keyboard_handler      ; 

global spurious_stub
spurious_stub:
    iretq
