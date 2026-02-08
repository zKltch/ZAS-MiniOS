ENTRY(_start)

SECTIONS {
    . = 1M;
   
    kernel_image_start = .;
    
    text_start = .;
    .text : {
        *(.multiboot_header)
        *(.text)
    }
    text_end = .;

    rodata_start = .;
    .rodata : {
        *(.rodata)
    }
    rodata_end = .;

    data_start = .;
    .data : {
        *(.data)
    }
    data_end = .;

    kernel_file_end = .;
    
    bss_start = .;
    .bss : {
        . = ALIGN(4096); 
        *(.bss)
    }
    bss_end = .;

    kernel_image_end = .;
}
