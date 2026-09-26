#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "types.h"

typedef enum {
    LAYOUT_US = 0,
    LAYOUT_DE = 1
} keyboard_layout_t;

// Common Scancodes (Set 1)
#define KEY_ESC       0x01
#define KEY_1         0x02
#define KEY_2         0x03
#define KEY_3         0x04
#define KEY_4         0x05
#define KEY_5         0x06
#define KEY_6         0x07
#define KEY_7         0x08
#define KEY_8         0x09
#define KEY_9         0x0A
#define KEY_0         0x0B
#define KEY_Q         0x10
#define KEY_W         0x11
#define KEY_E         0x12
#define KEY_R         0x13
#define KEY_T         0x14
#define KEY_Y         0x15
#define KEY_U         0x16
#define KEY_I         0x17
#define KEY_O         0x18
#define KEY_P         0x19
#define KEY_A         0x1E
#define KEY_S         0x1F
#define KEY_D         0x20
#define KEY_F         0x21
#define KEY_G         0x22
#define KEY_H         0x23
#define KEY_J         0x24
#define KEY_K         0x25
#define KEY_L         0x26
#define KEY_Z         0x2C
#define KEY_X         0x2D
#define KEY_C         0x2E
#define KEY_V         0x2F
#define KEY_B         0x30
#define KEY_N         0x31
#define KEY_M         0x32
#define KEY_SPACE     0x39
#define KEY_ENTER     0x1C

// Extended Keys (0xE0 scancode prefix)
#define KEY_EXT_BIT   0x80
#define KEY_UP        (0x48 | KEY_EXT_BIT)
#define KEY_LEFT      (0x4B | KEY_EXT_BIT)
#define KEY_RIGHT     (0x4D | KEY_EXT_BIT)
#define KEY_DOWN      (0x50 | KEY_EXT_BIT)

void keyboard_init(void);
void keyboard_set_layout(keyboard_layout_t layout);
void keyboard_toggle_layout(void);
keyboard_layout_t keyboard_get_layout(void);
const char *keyboard_get_layout_name(void);

// Blocking input
char getchar(void);
int readline(char *buf, int max_len);

// Non-blocking input (for games & real-time polling)
int keyboard_has_char(void);
char keyboard_getchar_async(void);

// Real-time key state (returns 1 if key is currently held down, 0 otherwise)
int keyboard_is_key_down(uint8_t scancode);

// Called by assembly ISR in kernel_entry.asm
void keyboard_isr_handler(void);

#endif
