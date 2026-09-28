bits 32
extern kmain
extern keyboard_isr_handler
extern timer_isr_handler
extern exception_handler
global _start
global isr_keyboard
global isr_timer
global isr_stub_table

_start:
    call kmain
    hlt
    jmp $

; ------------------------------------------------------------
; CPU Exception ISR Stubs (0-31)
; ------------------------------------------------------------
%macro ISR_NOERRCODE 1
isr%1:
    push dword 0        ; Dummy error code
    push dword %1       ; Interrupt vector number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
isr%1:
    push dword %1       ; Interrupt vector number (error code pushed by CPU)
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31

isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; Load kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Pass pointer to struct registers
    call exception_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8          ; Clean up interrupt vector and error code
    iret

isr_stub_table:
%assign i 0
%rep 32
    dd isr%+i
%assign i i+1
%endrep


isr_timer:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call timer_isr_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

isr_keyboard:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call keyboard_isr_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret
