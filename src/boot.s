%define KERNEL_VMA 0xFFFFFFFF80000000
%define LOW(x) ((x) - KERNEL_VMA)

section .multiboot_header
align 8
header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
    dw 0, 0, 8 
    dw 3, 0
    dd 16
    dq _start

    dw 0, 0, 8
header_end:

section .boot progbits alloc exec nowrite align=16
bits 32
global _start

_start:
    mov esp, LOW(stack_top)
    mov edi, ebx

    call setup_page_tables
    call enable_paging

    lgdt [LOW(gdt64.pointer)]  
    jmp gdt64.code_segment:long_mode_start

setup_page_tables:
    mov eax, LOW(p3_table)
    or eax, 0b11
    mov [LOW(p4_table)], eax

    mov eax, LOW(p2_table)
    or eax, 0b11
    mov [LOW(p3_table)], eax
    mov eax, LOW(p3_high)
    or eax, 0b11
    mov [LOW(p4_table) + 511 * 8], eax

    mov eax, LOW(p2_table)
    or eax, 0b11
    mov [LOW(p3_high) + 510 * 8], eax
    mov ecx, 0

.map_p2_loop:
    mov eax, 0x200000
    mul ecx             ; eax = 0x200000 * ecx

    or eax, 0b10000011
    mov [LOW(p2_table + ecx * 8)], eax

    inc ecx 
    cmp ecx, 128        ; 2M*128=256M
    jne .map_p2_loop
    ret

enable_paging:
    mov eax, LOW(p4_table)
    mov cr3, eax
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    or eax, 1 << 11 ; EFER.NXE
    wrmsr
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

bits 64

long_mode_start:                      
    mov rax, higher_half ; jmp to high half kernel
    jmp rax

section .text
bits 64
extern kmain

higher_half:
    lgdt [gdt64.pointer_hi]

    mov ax, gdt64.data_segment
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

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
p3_high:  resb 4096 
align 16
stack_bottom:
    resb 16384
stack_top:

section .rodata
gdt64:
    dq 0                                            ; null entry
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)  ; code (executable, present, long mode)
.data_segment: equ $ - gdt64
    dq (1 << 41) | (1 << 44) | (1 << 47)           ; data (writable, present)
.gdt_end: 
.pointer:
    dw gdt64.gdt_end - gdt64 - 1  
    dq LOW(gdt64)                
.pointer_hi:                    
    dw gdt64.gdt_end - gdt64 - 1
    dq gdt64


