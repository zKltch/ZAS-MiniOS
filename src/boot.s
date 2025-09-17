extern kernel_image_start,kernel_image_end,kernel_file_end

section .multiboot_header
header_start:

    dd 0xe85250d6 ; magic number
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start)) ; checksum
    dw 0, 0, 8
    ; --- Address Tag (type 2) ---
    dw 2    ; type
    dw 0    ; flags
    dd 40   ; size: 8 (tag header) + 4 * 8 (four 64-bit addresses) = 40
    dq header_start         ; header_addr
    dq kernel_image_start   ; load_addr
    dq kernel_file_end      ; load_end_addr (use file_end, not image_end)
    dq kernel_image_end     ; bss_end_addr
    ; --- Entry Point Tag (type 3) ---
    dw 3    ; type
    dw 0    ; flags
    dd 16   ; size: 8 (tag header) + 8 (one 64-bit address) = 16
    dq _start ; entry_addr

    ; --- End Tag (type 0) ---
    dw 0, 0, 8

header_end:

section .text
bits 64
global _start
extern kmain

_start:
    mov rdi, rbx
    mov rsp, stack_top
    mov rbp, 0
    call kmain
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB stack
stack_top: