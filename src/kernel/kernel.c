#include "types.h"
#include "idt.h"
#include "timer.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"

void kmain(void) {
    // 1. Initialize display
    vga_init();

    // 2. Initialize CPU interrupts (IDT & PIC)
    idt_init();

    // 3. Initialize hardware timer (PIT at 1000 Hz = 1ms resolution)
    timer_init(TIMER_FREQUENCY_HZ);

    // 4. Initialize keyboard driver
    keyboard_init();

    // 4. Launch interactive shell
    shell_run();

    // Halt if shell ever exits
    while (1) {
        __asm__ volatile("hlt");
    }
}
