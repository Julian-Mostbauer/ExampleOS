#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "types.h"

typedef enum {
    LAYOUT_US = 0,
    LAYOUT_DE = 1
} keyboard_layout_t;

void keyboard_init(void);
void keyboard_set_layout(keyboard_layout_t layout);
void keyboard_toggle_layout(void);
keyboard_layout_t keyboard_get_layout(void);
const char *keyboard_get_layout_name(void);

char getchar(void);
int readline(char *buf, int max_len);

// Called by assembly ISR in kernel_entry.asm
void keyboard_isr_handler(void);

#endif
