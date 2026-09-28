ENTRY(_start)
SECTIONS {
    /* The bootloader loads the kernel at 0x1000 */
    . = 0x1000;

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
    }

    . = ALIGN(4096);
    _kernel_end = .;
}
