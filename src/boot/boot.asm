bits 16
org 0x7c00

KERNEL_OFFSET equ 0x1000

start:
    ; Set up 16-bit real mode segments and stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [BOOT_DRIVE], dl    ; BIOS saves boot drive number in DL

    ; --------------------------------------------------------
    ; 1. Load C kernel from disk into RAM at KERNEL_OFFSET
    ; --------------------------------------------------------
    mov bx, KERNEL_OFFSET   ; ES:BX is target buffer (0x0000:0x1000)
    mov ah, 0x02            ; BIOS read sector function
    mov al, 31              ; Number of sectors to read (31 * 512 = 15.5 KB)
    mov ch, 0               ; Cylinder 0
    mov cl, 2               ; Sector 2 (Sector 1 is this bootloader)
    mov dh, 0               ; Head 0
    mov dl, [BOOT_DRIVE]    ; Drive number
    int 0x13
    jc disk_error

    ; --------------------------------------------------------
    ; 2. Switch from 16-bit Real Mode to 32-bit Protected Mode
    ; --------------------------------------------------------
    cli                     ; Disable interrupts during CPU mode switch
    lgdt [gdt_descriptor]   ; Load the Global Descriptor Table (GDT)

    mov eax, cr0            ; Set Protection Enable (PE) bit in Control Register 0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:init_pm    ; Far jump to flush CPU pipeline into 32-bit mode

disk_error:
    hlt
    jmp disk_error

; ------------------------------------------------------------
; Global Descriptor Table (GDT)
; ------------------------------------------------------------
gdt_start:
    dd 0x0, 0x0             ; Null descriptor (mandatory)

gdt_code:                   ; 32-bit Code Segment Descriptor
    dw 0xffff               ; Segment limit (bits 0-15): 4GB
    dw 0x0000               ; Base address (bits 0-15): 0x0
    db 0x00                 ; Base address (bits 16-23): 0x0
    db 10011010b            ; Flags: Present, Ring 0, Code, Exec/Read
    db 11001111b            ; Granularity: 4KB blocks, 32-bit mode, limit (16-19)
    db 0x00                 ; Base address (bits 24-31): 0x0

gdt_data:                   ; 32-bit Data Segment Descriptor
    dw 0xffff               ; Segment limit: 4GB
    dw 0x0000               ; Base address: 0x0
    db 0x00                 ; Base address: 0x0
    db 10010010b            ; Flags: Present, Ring 0, Data, Read/Write
    db 11001111b            ; Granularity: 4KB blocks, 32-bit mode
    db 0x00                 ; Base address: 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size - 1
    dd gdt_start                ; GDT address

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ------------------------------------------------------------
; 32-bit Protected Mode
; ------------------------------------------------------------
bits 32
init_pm:
    ; Point all segment registers to our 32-bit data segment
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up 32-bit stack in free memory space
    mov esp, 0x90000

    ; Jump to our loaded C kernel!
    call KERNEL_OFFSET

    ; If kernel returns, halt
    hlt
    jmp $

BOOT_DRIVE: db 0

; Pad to 510 bytes + 2 bytes boot signature
times 510 - ($ - $$) db 0
dw 0xaa55
