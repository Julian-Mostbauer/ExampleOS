#include "timer.h"
#include "io.h"
#include "pic.h"

// 8253/8254 Programmable Interval Timer (PIT) I/O ports
#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND_PORT  0x43
#define PIT_BASE_FREQUENCY 1193182

static volatile uint32_t timer_ticks = 0;

void timer_init(uint32_t frequency) {
    if (frequency == 0) {
        frequency = TIMER_FREQUENCY_HZ;
    }

    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;

    // Command: Channel 0, Access mode lobyte/hibyte, Mode 3 (Square wave), 16-bit binary
    outb(PIT_COMMAND_PORT, 0x36);
    outb(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_isr_handler(void) {
    timer_ticks++;
    pic_send_eoi(0);
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

void sleep_ms(uint32_t ms) {
    if (ms == 0) return;

    uint32_t start = timer_ticks;
    while ((timer_ticks - start) < ms) {
        // Halt CPU until next interrupt (timer tick or keyboard).
        // This does NOT busy-wait: the CPU idles in low-power halt state.
        __asm__ volatile("sti; hlt");
    }
}

void sleep(uint32_t seconds) {
    sleep_ms(seconds * 1000);
}
