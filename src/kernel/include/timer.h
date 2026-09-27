#ifndef _TIMER_H_
#define _TIMER_H_

#include "types.h"

#define TIMER_FREQUENCY_HZ 1000

// Initialize PIT Channel 0 at given frequency (e.g. 1000 Hz = 1 ms per tick)
void timer_init(uint32_t frequency);

// Get total elapsed ticks since boot (in milliseconds if frequency is 1000 Hz)
uint32_t timer_get_ticks(void);

// Non-busy-waiting sleep: puts CPU in low-power halt ('hlt') state until time expires
void sleep_ms(uint32_t ms);
void sleep(uint32_t seconds);

// Called by assembly ISR in kernel_entry.asm
void timer_isr_handler(void);

#endif
