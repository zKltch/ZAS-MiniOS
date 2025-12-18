ENTRY(_start)

SECTIONS {
    . = 1M;
    
    kernel_image_start = .;

    .text : {
        *(.multiboot_header)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    kernel_file_end = .;

    .bss : {
        . = ALIGN(4096); 
        *(.bss)
    }

    kernel_image_end = .;
}