ENTRY(_start)


KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = 1M;

    .boot : {
        *(.multiboot_header)
        *(.boot)
    }  
    . = ALIGN(4K);
    . += KERNEL_VMA; 

    kernel_image_start = .;
    
    text_start = .;
    .text : AT(ADDR(.text) - KERNEL_VMA) {
        *(.text)
    }
    . = ALIGN(4K);
    text_end = .;

    rodata_start = .;
    .rodata : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata)
    }
    . = ALIGN(4K);
    rodata_end = .;

    data_start = .;
    .data : AT(ADDR(.data) - KERNEL_VMA) {
        *(.data)
    }
    . = ALIGN(4K);
    data_end = .;

    kernel_file_end = .;
    
    bss_start = .;
    .bss : AT(ADDR(.bss) - KERNEL_VMA) {
        *(COMMON)
        *(.bss)
    }
    . = ALIGN(4K);
    bss_end = .;

    kernel_image_end = .;
}
