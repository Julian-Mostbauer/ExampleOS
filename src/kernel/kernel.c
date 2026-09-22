// kernel.c - 32-bit Protected Mode Kernel with ASCII Keyboard & Shell
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define VGA_ADDRESS    0xB8000
#define VGA_WIDTH      80
#define VGA_HEIGHT     25

#define COLOR_BLACK    0x0
#define COLOR_GREEN    0x2
#define COLOR_CYAN     0x3
#define COLOR_LIGHT_GRAY 0x7
#define COLOR_YELLOW   0xE
#define COLOR_WHITE    0xF

#define MAKE_COLOR(fg, bg) ((uint8_t)((bg << 4) | (fg & 0x0F)))

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
// VGA Text Driver with Scrolling & Cursor Tracking
// ------------------------------------------------------------
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t current_color = MAKE_COLOR(COLOR_WHITE, COLOR_BLACK);

void clear_screen(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i * 2]     = ' ';
        video[i * 2 + 1] = current_color;
    }
    cursor_row = 0;
    cursor_col = 0;
}

static void scroll(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    if (cursor_row >= VGA_HEIGHT) {
        // Shift lines up by 1
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i++) {
            video[i] = video[i + VGA_WIDTH * 2];
        }
        // Clear the bottom line
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
        return;
    }

    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            int offset = (cursor_row * VGA_WIDTH + cursor_col) * 2;
            video[offset]     = ' ';
            video[offset + 1] = current_color;
        }
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
// Circular Keyboard Ring Buffer (Queue)
// ------------------------------------------------------------
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_head = 0;
static volatile int kb_tail = 0;

static void kb_push(char c) {
    int next = (kb_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_tail) { // Don't overflow
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

// Blocking read: waits until a key is pressed via interrupts
char getchar(void) {
    while (kb_head == kb_tail) {
        // Sleep CPU until the next interrupt arrives
        __asm__ volatile("hlt");
    }
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

// Reads a whole line of ASCII text into buffer, handles Backspace and Enter
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
        } else if (c >= ' ' && c <= '~') {
            if (count < max_len - 1) {
                buf[count++] = c;
                putchar(c); // Echo character to screen
            }
        }
    }
}

// ------------------------------------------------------------
// Scancode Translation with Shift Support
// ------------------------------------------------------------
static int shift_active = 0;
static int caps_lock    = 0;

static const char scancode_ascii_normal[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' '
};

static const char scancode_ascii_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' '
};

void keyboard_handler_main(void) {
    uint8_t scancode = inb(0x60);

    // Track Shift key press (0x2A = Left Shift, 0x36 = Right Shift)
    if (scancode == 0x2A || scancode == 0x36) {
        shift_active = 1;
    }
    // Track Shift key release (0xAA = Left Shift release, 0xB6 = Right Shift release)
    else if (scancode == 0xAA || scancode == 0xB6) {
        shift_active = 0;
    }
    // Track Caps Lock toggle (0x3A)
    else if (scancode == 0x3A) {
        caps_lock = !caps_lock;
    }
    // Key press event (bit 7 is 0)
    else if (!(scancode & 0x80)) {
        char ch = 0;
        int upper = shift_active ^ caps_lock;

        if (upper) {
            ch = scancode_ascii_shift[scancode];
        } else {
            ch = scancode_ascii_normal[scancode];
        }

        if (ch) {
            kb_push(ch); // Place into ring buffer for getchar()
        }
    }

    // Send End of Interrupt (EOI) to Master PIC
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
    idt[n].selector    = 0x08; // Code segment selector
    idt[n].zero        = 0;
    idt[n].type_attr   = 0x8E; // 32-bit Interrupt Gate
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
// Simple Interactive Shell
// ------------------------------------------------------------
void kmain(void) {
    clear_screen();

    print_color("=================================================================\n", MAKE_COLOR(COLOR_CYAN, COLOR_BLACK));
    print_color("               SimpleOS 32-bit Interactive Shell                 \n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
    print_color("=================================================================\n", MAKE_COLOR(COLOR_CYAN, COLOR_BLACK));
    print_color("Type 'help' to see available commands.\n\n", MAKE_COLOR(COLOR_LIGHT_GRAY, COLOR_BLACK));

    char input[64];

    // Initialize IDT and enable hardware interrupts
    idt_init();

    while (1) {
        print_color("os> ", MAKE_COLOR(COLOR_GREEN, COLOR_BLACK));
        readline(input, sizeof(input));

        if (input[0] == '\0') {
            continue;
        } else if (strcmp(input, "help") == 0) {
            print("Available commands:\n");
            print("  help    - Show this help message\n");
            print("  clear   - Clear the screen\n");
            print("  about   - Display system info\n");
            print("  echo    - Repeat back a greeting\n");
        } else if (strcmp(input, "clear") == 0) {
            clear_screen();
        } else if (strcmp(input, "about") == 0) {
            print_color("SimpleOS v0.1\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            print("Architecture: x86 32-bit Protected Mode\n");
            print("Drivers: VGA Text Buffer (0xB8000), PS/2 Keyboard IRQ 1\n");
        } else if (strcmp(input, "echo") == 0) {
            print("Echo: Hello from your custom OS kernel!\n");
        } else {
            print_color("Unknown command: '", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
            print_color(input, MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            print_color("'. Type 'help' for commands.\n", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
        }
    }
}
