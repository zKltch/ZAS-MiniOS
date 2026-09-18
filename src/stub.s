bits 64
section .text

global PitTimer_stub
extern PitTimer_handler
extern current
extern FirstEnterTask

PitTimer_stub:
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    ;sub rsp, 8
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov rax, [rel current]
    push rax

    mov rbx, rsp
    sub rsp, 0x8
    and rsp, -16

    cld
    call PitTimer_handler

    mov rsp, rbx

    pop rax                 ;prev task
    mov rcx, [rel current]  ;next task

    test rax, rax           ;current == NULL
    jz .NoTask

    mov rdx, [rel FirstEnterTask]
    test rdx, rdx           ;FirstEnterTask == True
    jnz .First

    mov [rax], rsp
    mov rsp, [rcx]

    mov rdx, [rcx + 16]
    mov cr3, rdx
    
    jmp .Restore

    .First:
    mov qword [rel FirstEnterTask], 0

    mov rsp, [rcx]

    mov rdx, [rcx + 16]
    mov cr3, rdx

    .NoTask:
    .Restore:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ;add rsp, 8
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
