#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "vga.h"

static volatile keyboard_layout_t current_layout = LAYOUT_DE;

static int shift_active = 0;
static int caps_lock    = 0;
static int is_extended  = 0;
static int altgr_active = 0;

// Circular buffer
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_head = 0;
static volatile int kb_tail = 0;

// Key state table (1 = held down, 0 = released).
// 0..127: Normal keys. 128..255: Extended keys (e.g. arrow keys).
static volatile uint8_t key_states[256];

// --- US QWERTY Tables ---
static const char scancode_us_normal[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' '
};

static const char scancode_us_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' '
};

// --- German Standard QWERTZ Tables (CP437) ---
// ä = 0x84, ö = 0x94, ü = 0x81, ß = 0xE1
// Ä = 0x8E, Ö = 0x99, Ü = 0x9A, § = 0x15, ° = 0xF8
static const char scancode_de_normal[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', (char)0xE1, '`', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', (char)0x81, '+', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', (char)0x94, (char)0x84, '^',
    0,   '#', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '<'
};

static const char scancode_de_shift[128] = {
    0,   27,  '!', '"', (char)0x15, '$', '%', '&', '/', '(', ')', '=', '?', '`', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', (char)0x9A, '*', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', (char)0x99, (char)0x8E, (char)0xF8,
    0,   '\'', 'Y', 'X', 'C', 'V', 'B', 'N', 'M', ';', ':', '_', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '>'
};

static const char scancode_de_altgr[128] = {
    0,   0,   0,   (char)0xFD, 0,   0,   0,   0,   '{', '[', ']', '}', '\\', 0,   0,
    0,   '@', 0,   'E', 0,   0,   0,   0,   0,   0,   0,   0,   '~', 0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   (char)0xE6, 0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '|'
};

static void kb_push(char c) {
    int next = (kb_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

char getchar(void) {
    while (kb_head == kb_tail) {
        __asm__ volatile("hlt");
    }
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

int keyboard_has_char(void) {
    return kb_head != kb_tail;
}

char keyboard_getchar_async(void) {
    if (kb_head == kb_tail) {
        return 0;
    }
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

int keyboard_is_key_down(uint8_t scancode) {
    return key_states[scancode];
}

int readline(char *buf, int max_len) {
    int count = 0;
    while (1) {
        char c = getchar();

        if (c == '\n') {
            vga_putchar('\n');
            buf[count] = '\0';
            return count;
        } else if (c == '\b') {
            if (count > 0) {
                count--;
                vga_putchar('\b');
            }
        } else if ((uint8_t)c >= ' ' && (uint8_t)c != 127) {
            if (count < max_len - 1) {
                buf[count++] = c;
                vga_putchar(c);
            }
        }
    }
}

void keyboard_set_layout(keyboard_layout_t layout) {
    current_layout = layout;
    if (current_layout == LAYOUT_DE) {
        vga_set_status_badge("[Layout: DE (QWERTZ)] (F1) ");
    } else {
        vga_set_status_badge("[Layout: US (QWERTY)] (F1) ");
    }
}

void keyboard_toggle_layout(void) {
    if (current_layout == LAYOUT_DE) {
        keyboard_set_layout(LAYOUT_US);
    } else {
        keyboard_set_layout(LAYOUT_DE);
    }
}

keyboard_layout_t keyboard_get_layout(void) {
    return current_layout;
}

const char *keyboard_get_layout_name(void) {
    return (current_layout == LAYOUT_DE) ? "German (DE - QWERTZ)" : "US (QWERTY)";
}

void keyboard_isr_handler(void) {
    uint8_t scancode = inb(0x60);

    // Extended scancode (0xE0 prefix, e.g. arrow keys, AltGr)
    if (scancode == 0xE0) {
        is_extended = 1;
        pic_send_eoi(1);
        return;
    }

    if (is_extended) {
        is_extended = 0;
        uint8_t make_code = scancode & 0x7F;
        uint8_t pressed = !(scancode & 0x80);

        // Record extended key state in high half of table (128..255)
        key_states[make_code | KEY_EXT_BIT] = pressed;

        if (scancode == 0x38) {
            altgr_active = 1;
        } else if (scancode == 0xB8) {
            altgr_active = 0;
        }
        pic_send_eoi(1);
        return;
    }

    // Normal keys (0..127)
    uint8_t make_code = scancode & 0x7F;
    uint8_t pressed = !(scancode & 0x80);
    key_states[make_code] = pressed;

    // Shift
    if (scancode == 0x2A || scancode == 0x36) {
        shift_active = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_active = 0;
    }
    // Caps Lock
    else if (scancode == 0x3A && pressed) {
        caps_lock = !caps_lock;
    }
    // F1: Hotkey to toggle layout
    else if (scancode == 0x3B && pressed) {
        keyboard_toggle_layout();
    }
    // Normal key press (Make code only)
    else if (pressed && make_code < 128) {
        char ch = 0;
        int upper = shift_active ^ caps_lock;

        if (current_layout == LAYOUT_DE) {
            if (altgr_active) {
                ch = scancode_de_altgr[make_code];
            } else if (upper) {
                ch = scancode_de_shift[make_code];
            } else {
                ch = scancode_de_normal[make_code];
            }
        } else {
            if (upper) {
                ch = scancode_us_shift[make_code];
            } else {
                ch = scancode_us_normal[make_code];
            }
        }

        if (ch) {
            kb_push(ch);
        }
    }

    pic_send_eoi(1);
}

void keyboard_init(void) {
    for (int i = 0; i < 256; i++) {
        key_states[i] = 0;
    }
    kb_head = 0;
    kb_tail = 0;
    keyboard_set_layout(LAYOUT_DE);
}
