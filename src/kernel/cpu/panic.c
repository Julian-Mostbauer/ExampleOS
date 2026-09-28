#include "idt.h"
#include "vga.h"
#include "io_helper.h"

static const char *exception_messages[32] = {
    "Division By Zero (#DE)",
    "Debug (#DB)",
    "Non-Maskable Interrupt (NMI)",
    "Breakpoint (#BP)",
    "Into Detected Overflow (#OF)",
    "Out of Bounds (#BR)",
    "Invalid Opcode (#UD)",
    "No Coprocessor (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Bad TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack Fault (#SS)",
    "General Protection Fault (#GP)",
    "Page Fault (#PF)",
    "Unknown Interrupt",
    "x87 FPU Floating-Point Error (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point Exception (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection Exception (#CP)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception (#HV)",
    "VMM Communication Exception (#VC)",
    "Security Exception (#SX)",
    "Reserved"
};

void exception_handler(struct registers *regs) {
    // 1. Disable interrupts immediately
    __asm__ volatile("cli");

    // 2. Switch back to 80x25 text mode in case exception occurred in graphics mode
    vga_set_mode_03h();
    vga_clear();

    // 3. Render Kernel Panic Screen
    uint8_t header_color = MAKE_COLOR(COLOR_WHITE, COLOR_RED);
    uint8_t alert_color  = MAKE_COLOR(COLOR_LIGHT_RED, COLOR_BLACK);
    uint8_t text_color   = MAKE_COLOR(COLOR_LIGHT_GRAY, COLOR_BLACK);
    uint8_t val_color    = MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK);

    vga_print_color("\n ========================== KERNEL PANIC ==========================\n\n", header_color);

    const char *name = (regs->int_no < 32) ? exception_messages[regs->int_no] : "Unknown Exception";
    vga_print_color(" Exception: ", alert_color);
    vga_print_color(name, val_color);
    vga_print("\n");

    char buf[96];
    sprintf(buf, " Vector:    0x%x (%u)    Error Code: 0x%x\n\n", regs->int_no, regs->int_no, regs->err_code);
    vga_print_color(buf, text_color);

    if (regs->int_no == 14) { // Page Fault
        uint32_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        sprintf(buf, " Faulting Address (CR2): 0x%x\n\n", cr2);
        vga_print_color(buf, alert_color);
    }

    vga_print_color(" --- Register Dump ---\n", MAKE_COLOR(COLOR_CYAN, COLOR_BLACK));
    sprintf(buf, " EAX: 0x%x   EBX: 0x%x   ECX: 0x%x   EDX: 0x%x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    vga_print_color(buf, text_color);

    sprintf(buf, " ESI: 0x%x   EDI: 0x%x   EBP: 0x%x   ESP: 0x%x\n", regs->esi, regs->edi, regs->ebp, regs->esp);
    vga_print_color(buf, text_color);

    sprintf(buf, " EIP: 0x%x   CS:  0x%x   EFLAGS: 0x%x\n", regs->eip, regs->cs, regs->eflags);
    vga_print_color(buf, text_color);

    sprintf(buf, " DS:  0x%x   ES:  0x%x   FS:  0x%x   GS:  0x%x\n\n", regs->ds, regs->es, regs->fs, regs->gs);
    vga_print_color(buf, text_color);

    vga_print_color(" System halted. Please restart your machine.\n", header_color);

    // 4. Halt CPU execution
    while (1) {
        __asm__ volatile("hlt");
    }
}
