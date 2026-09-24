// kernel.c - 32-bit Protected Mode Kernel with Multilingual Keyboard Support
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define VGA_ADDRESS      0xB8000
#define VGA_WIDTH        80
#define VGA_HEIGHT       25

#define COLOR_BLACK      0x0
#define COLOR_BLUE       0x1
#define COLOR_GREEN      0x2
#define COLOR_CYAN       0x3
#define COLOR_RED        0x4
#define COLOR_MAGENTA    0x5
#define COLOR_BROWN      0x6
#define COLOR_LIGHT_GRAY 0x7
#define COLOR_DARK_GRAY  0x8
#define COLOR_LIGHT_BLUE 0x9
#define COLOR_LIGHT_GREEN 0xA
#define COLOR_LIGHT_CYAN 0xB
#define COLOR_LIGHT_RED  0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW     0xE
#define COLOR_WHITE      0xF

#define MAKE_COLOR(fg, bg) ((uint8_t)(((bg) << 4) | ((fg) & 0x0F)))

// ------------------------------------------------------------
// Low-level I/O Ports
// ------------------------------------------------------------
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

// ------------------------------------------------------------
// Keyboard Layout Definitions
// ------------------------------------------------------------
typedef enum {
    LAYOUT_US = 0,
    LAYOUT_DE = 1
} keyboard_layout_t;

static volatile keyboard_layout_t current_layout = LAYOUT_DE; // Default to German

// Forward declaration
void draw_status_bar(void);

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

// --- German Standard QWERTZ Tables (CP437 character codes) ---
// ä = 0x84, ö = 0x94, ü = 0x81, ß = 0xE1
// Ä = 0x8E, Ö = 0x99, Ü = 0x9A, § = 0x15, ° = 0xF8
static const char scancode_de_normal[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', (char)0xE1, '`', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', (char)0x81, '+', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', (char)0x94, (char)0x84, '^',
    0,   '#', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '<' // 0x56 = ISO key '<'
};

static const char scancode_de_shift[128] = {
    0,   27,  '!', '"', (char)0x15, '$', '%', '&', '/', '(', ')', '=', '?', '`', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', (char)0x9A, '*', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', (char)0x99, (char)0x8E, (char)0xF8,
    0,   '\'', 'Y', 'X', 'C', 'V', 'B', 'N', 'M', ';', ':', '_', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '>' // 0x56 = ISO key '>'
};

static const char scancode_de_altgr[128] = {
    0,   0,   0,   (char)0xFD, 0,   0,   0,   0,   '{', '[', ']', '}', '\\', 0,   0,
    0,   '@', 0,   'E', 0,   0,   0,   0,   0,   0,   0,   0,   '~', 0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   (char)0xE6, 0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '|' // 0x56 = ISO key '|'
};

// ------------------------------------------------------------
// VGA Text Driver with Scrolling & Pinned Status Bar
// ------------------------------------------------------------
static int cursor_row = 1;
static int cursor_col = 0;
static uint8_t current_color = MAKE_COLOR(COLOR_WHITE, COLOR_BLACK);

void draw_status_bar(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    uint8_t bar_color   = MAKE_COLOR(COLOR_WHITE, COLOR_BLUE);
    uint8_t badge_color = MAKE_COLOR(COLOR_YELLOW, COLOR_BLUE);

    // Row 0: Background
    for (int col = 0; col < VGA_WIDTH; col++) {
        video[col * 2]     = ' ';
        video[col * 2 + 1] = bar_color;
    }

    // Title on left
    const char *title = " SimpleOS 32-bit Kernel ";
    for (int i = 0; title[i] != '\0' && i < VGA_WIDTH; i++) {
        video[i * 2]     = title[i];
        video[i * 2 + 1] = bar_color;
    }

    // Layout badge on right
    const char *badge = (current_layout == LAYOUT_DE) ? "[Layout: DE (QWERTZ)] (F1) " : "[Layout: US (QWERTY)] (F1) ";
    int start_col = VGA_WIDTH - 27;
    for (int i = 0; badge[i] != '\0' && (start_col + i) < VGA_WIDTH; i++) {
        int offset = (start_col + i) * 2;
        video[offset]     = badge[i];
        video[offset + 1] = badge_color;
    }
}

// ------------------------------------------------------------
// VGA Hardware Cursor Control
// ------------------------------------------------------------
void update_cursor(int row, int col) {
    uint16_t pos = row * VGA_WIDTH + col;

    // Send low byte to CRTC register 0x0F
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    // Send high byte to CRTC register 0x0E
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    // Configure cursor scanlines (shape) and enable blinking
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void clear_screen(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;

    // Clear all rows from row 1 down to row 24
    for (int i = VGA_WIDTH; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i * 2]     = ' ';
        video[i * 2 + 1] = current_color;
    }

    cursor_row = 1;
    cursor_col = 0;
    draw_status_bar();
    update_cursor(cursor_row, cursor_col);
}

static void scroll(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    if (cursor_row >= VGA_HEIGHT) {
        // Shift lines 1..23 up by 1 (preserving row 0 status bar)
        for (int r = 1; r < VGA_HEIGHT - 1; r++) {
            for (int c = 0; c < VGA_WIDTH * 2; c++) {
                video[(r * VGA_WIDTH * 2) + c] = video[((r + 1) * VGA_WIDTH * 2) + c];
            }
        }
        // Clear bottom line
        int last_line = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
        for (int i = 0; i < VGA_WIDTH; i++) {
            video[last_line + i * 2]     = ' ';
            video[last_line + i * 2 + 1] = current_color;
        }
        cursor_row = VGA_HEIGHT - 1;
    }
}

void putchar(char c) {
    volatile char *video = (volatile char *)VGA_ADDRESS;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        scroll();
        update_cursor(cursor_row, cursor_col);
        return;
    }

    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            int offset = (cursor_row * VGA_WIDTH + cursor_col) * 2;
            video[offset]     = ' ';
            video[offset + 1] = current_color;
        }
        update_cursor(cursor_row, cursor_col);
        return;
    }

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
        scroll();
    }

    int offset = (cursor_row * VGA_WIDTH + cursor_col) * 2;
    video[offset]     = c;
    video[offset + 1] = current_color;
    cursor_col++;
    update_cursor(cursor_row, cursor_col);
}

void print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }
}

void print_color(const char *str, uint8_t color) {
    uint8_t old = current_color;
    current_color = color;
    print(str);
    current_color = old;
}

// ------------------------------------------------------------
// Circular Keyboard Ring Buffer
// ------------------------------------------------------------
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_head = 0;
static volatile int kb_tail = 0;

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

int readline(char *buf, int max_len) {
    int count = 0;
    while (1) {
        char c = getchar();

        if (c == '\n') {
            putchar('\n');
            buf[count] = '\0';
            return count;
        } else if (c == '\b') {
            if (count > 0) {
                count--;
                putchar('\b');
            }
        } else if ((uint8_t)c >= ' ' && (uint8_t)c != 127) {
            // Allows standard ASCII and extended CP437 German umlauts
            if (count < max_len - 1) {
                buf[count++] = c;
                putchar(c);
            }
        }
    }
}

// ------------------------------------------------------------
// Keyboard Interrupt Handler with Layout & AltGr Support
// ------------------------------------------------------------
static int shift_active = 0;
static int caps_lock    = 0;
static int is_extended  = 0;
static int altgr_active = 0;

void set_keyboard_layout(keyboard_layout_t layout) {
    current_layout = layout;
    draw_status_bar();
}

void toggle_keyboard_layout(void) {
    if (current_layout == LAYOUT_US) {
        set_keyboard_layout(LAYOUT_DE);
    } else {
        set_keyboard_layout(LAYOUT_US);
    }
}

void keyboard_handler_main(void) {
    uint8_t scancode = inb(0x60);

    // Extended scancode prefix (AltGr is 0xE0 0x38)
    if (scancode == 0xE0) {
        is_extended = 1;
        outb(0x20, 0x20);
        return;
    }

    if (is_extended) {
        is_extended = 0;
        if (scancode == 0x38) {
            altgr_active = 1; // AltGr pressed
        } else if (scancode == 0xB8) {
            altgr_active = 0; // AltGr released
        }
        outb(0x20, 0x20);
        return;
    }

    // Shift Key
    if (scancode == 0x2A || scancode == 0x36) {
        shift_active = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_active = 0;
    }
    // Caps Lock Key
    else if (scancode == 0x3A) {
        caps_lock = !caps_lock;
    }
    // F1 Key: Instant Layout Switch Hotkey
    else if (scancode == 0x3B) {
        toggle_keyboard_layout();
    }
    // Normal Key Press (bit 7 is 0)
    else if (!(scancode & 0x80) && scancode < 128) {
        char ch = 0;
        int upper = shift_active ^ caps_lock;

        if (current_layout == LAYOUT_DE) {
            if (altgr_active) {
                ch = scancode_de_altgr[scancode];
            } else if (upper) {
                ch = scancode_de_shift[scancode];
            } else {
                ch = scancode_de_normal[scancode];
            }
        } else {
            // US Layout
            if (upper) {
                ch = scancode_us_shift[scancode];
            } else {
                ch = scancode_us_normal[scancode];
            }
        }

        if (ch) {
            kb_push(ch);
        }
    }

    // Send EOI to PIC
    outb(0x20, 0x20);
}

// ------------------------------------------------------------
// IDT & PIC Initialization
// ------------------------------------------------------------
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

extern void isr_keyboard(void);

static void set_idt_gate(int n, uint32_t handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].zero        = 0;
    idt[n].type_attr   = 0x8E;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

static void pic_remap(void) {
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFD); // Only IRQ 1 (keyboard) enabled
    outb(0xA1, 0xFF);
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;

    pic_remap();
    set_idt_gate(0x21, (uint32_t)isr_keyboard);

    __asm__ volatile("lidt %0" : : "m"(idtp));
    __asm__ volatile("sti");
}

// ------------------------------------------------------------
// String Helpers
// ------------------------------------------------------------
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// ------------------------------------------------------------
// Shell & Main Entry
// ------------------------------------------------------------
void kmain(void) {
    enable_cursor(14, 15);
    clear_screen();

    print_color("Welcome to SimpleOS!\n", MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    print_color("Keyboard: German Standard (QWERTZ) active. Press F1 to toggle.\n", MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    print_color("Type 'help' to see commands, or 'layout [de|us]' to change.\n\n", MAKE_COLOR(COLOR_LIGHT_GRAY, COLOR_BLACK));

    char input[64];

    idt_init();

    while (1) {
        print_color("os> ", MAKE_COLOR(COLOR_GREEN, COLOR_BLACK));
        readline(input, sizeof(input));

        if (input[0] == '\0') {
            continue;
        } else if (strcmp(input, "help") == 0) {
            print("Available commands:\n");
            print("  layout [de|us] - View or switch keyboard layout (or press F1)\n");
            print("  clear          - Clear the screen\n");
            print("  about          - Display system information\n");
            print("  echo           - Repeat greeting message\n");
            print("  help           - Show this help menu\n");
        } else if (strcmp(input, "layout de") == 0 || strcmp(input, "de") == 0) {
            set_keyboard_layout(LAYOUT_DE);
            print_color("Keyboard layout switched to German (DE - QWERTZ).\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
        } else if (strcmp(input, "layout us") == 0 || strcmp(input, "us") == 0) {
            set_keyboard_layout(LAYOUT_US);
            print_color("Keyboard layout switched to US (QWERTY).\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
        } else if (strcmp(input, "layout") == 0) {
            if (current_layout == LAYOUT_DE) {
                print_color("Current layout: German (DE - QWERTZ).\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            } else {
                print_color("Current layout: US (QWERTY).\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            }
            print("Usage: 'layout de' or 'layout us' (or press F1 anytime).\n");
        } else if (strcmp(input, "clear") == 0) {
            clear_screen();
        } else if (strcmp(input, "about") == 0) {
            print_color("SimpleOS v0.2\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            print("Architecture: x86 32-bit Protected Mode\n");
            print("Supported Layouts: German (DE QWERTZ) & US (QWERTY)\n");
            print("Features: CP437 Umlauts (ae, oe, ue, ss), AltGr symbols, IDT IRQ 1\n");
        } else if (strcmp(input, "echo") == 0) {
            print("Echo: Hallo aus dem Betriebssystem-Kernel!\n");
        } else {
            print_color("Unknown command: '", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
            print_color(input, MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            print_color("'. Type 'help' for commands.\n", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
        }
    }
}
