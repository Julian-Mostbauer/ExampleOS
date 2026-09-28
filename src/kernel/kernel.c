#include "idt.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"

void kmain(void) {
    vga_init();

    idt_init();

    pmm_init();

    timer_init(TIMER_FREQUENCY_HZ);

    keyboard_init();

    shell_run();

    // Halt if shell ever exits
    while (1) {
        __asm__ volatile("hlt");
    }
}
