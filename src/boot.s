extern kernel_image_start,kernel_image_end,kernel_file_end

section .multiboot_header
align 8
header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
    dw 0, 0, 8
    
    dw 2, 0
    dd 40
    dq header_start
    dq kernel_image_start
    dq kernel_file_end
    dq kernel_image_end
    
    dw 3, 0
    dd 16
    dq _start

    dw 0, 0, 8
header_end:

section .text
bits 32
global _start

_start:
    mov esp, stack_top
    mov edi, ebx

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start

setup_page_tables:
    mov eax, p3_table
    or eax, 0b11
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    mov eax, 0x0
    or eax, 0b10000011
    mov [p2_table], eax
    ret

enable_paging:
    mov eax, p4_table
    mov cr3, eax
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

bits 64
extern kmain

long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rdi, rdi
    mov rdi, rbx

    mov rsp, stack_top
    mov rbp, 0
    call kmain
    
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 4096
p4_table: resb 4096
p3_table: resb 4096
p2_table: resb 4096
align 16
stack_bottom:
    resb 16384
stack_top:

section .rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
