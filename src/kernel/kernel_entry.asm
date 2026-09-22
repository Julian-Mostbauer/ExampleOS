bits 32
extern kmain
global _start

_start:
    call kmain      ; Call the C function
    hlt             ; Halt CPU if kmain returns
    jmp $
