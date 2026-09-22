bits 32
extern kmain
extern keyboard_handler_main
global _start
global isr_keyboard

_start:
    call kmain
    hlt
    jmp $

isr_keyboard:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10    ; Load kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call keyboard_handler_main

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret
