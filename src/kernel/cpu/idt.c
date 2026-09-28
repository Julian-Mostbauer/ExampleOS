#include "idt.h"
#include "pic.h"

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

extern void isr_timer(void);
extern void isr_keyboard(void);
extern void *isr_stub_table[32];

void idt_set_gate(int n, uint32_t handler) {
    idt[n].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[n].selector    = 0x08; // Kernel code segment selector
    idt[n].zero        = 0;
    idt[n].type_attr   = 0x8E; // 32-bit Interrupt Gate, Ring 0, Present
    idt[n].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;

    // Register CPU exception handlers (vectors 0..31)
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (uint32_t)isr_stub_table[i]);
    }

    // Remap PIC before loading IDT
    pic_remap();

    // Register IRQ0 (vector 0x20 = 32, Timer) and IRQ1 (vector 0x21 = 33, Keyboard)
    idt_set_gate(0x20, (uint32_t)isr_timer);
    idt_set_gate(0x21, (uint32_t)isr_keyboard);

    // Load IDT register
    __asm__ volatile("lidt %0" : : "m"(idtp));

    // Enable CPU interrupts
    __asm__ volatile("sti");
}
