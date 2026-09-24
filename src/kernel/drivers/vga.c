#include "vga.h"
#include "io.h"

static int cursor_row = 1;
static int cursor_col = 0;
static uint8_t current_color = MAKE_COLOR(COLOR_WHITE, COLOR_BLACK);
static char status_badge[32] = "[Layout: DE (QWERTZ)] (F1) ";

void vga_set_status_badge(const char *badge) {
    int i = 0;
    while (badge[i] != '\0' && i < 31) {
        status_badge[i] = badge[i];
        i++;
    }
    status_badge[i] = '\0';
    vga_draw_status_bar();
}

void vga_update_cursor(int row, int col) {
    uint16_t pos = row * VGA_WIDTH + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_enable_cursor(uint8_t start, uint8_t end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

void vga_disable_cursor(void)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void vga_draw_status_bar(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    uint8_t bar_color   = MAKE_COLOR(COLOR_WHITE, COLOR_BLUE);
    uint8_t badge_color = MAKE_COLOR(COLOR_YELLOW, COLOR_BLUE);

    // Row 0 background
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

    // Badge on right
    int len = 0;
    while (status_badge[len] != '\0') len++;

    int start_col = VGA_WIDTH - len - 1;
    if (start_col < 25) start_col = 25;

    for (int i = 0; status_badge[i] != '\0' && (start_col + i) < VGA_WIDTH; i++) {
        int offset = (start_col + i) * 2;
        video[offset]     = status_badge[i];
        video[offset + 1] = badge_color;
    }
}

void vga_clear(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;

    // Clear all rows from row 1 down to row 24
    for (int i = VGA_WIDTH; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i * 2]     = ' ';
        video[i * 2 + 1] = current_color;
    }

    cursor_row = 1;
    cursor_col = 0;
    vga_draw_status_bar();
    vga_update_cursor(cursor_row, cursor_col);
}

void vga_clear_absolute(void){
    volatile char *video = (volatile char *)VGA_ADDRESS;

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i * 2]     = ' ';
        video[i * 2 + 1] = current_color;
    }

    vga_disable_cursor();
}

static void vga_scroll(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    if (cursor_row >= VGA_HEIGHT) {
        // Shift rows 1..23 up by 1 (preserving row 0 status bar)
        for (int r = 1; r < VGA_HEIGHT - 1; r++) {
            for (int c = 0; c < VGA_WIDTH * 2; c++) {
                video[(r * VGA_WIDTH * 2) + c] = video[((r + 1) * VGA_WIDTH * 2) + c];
            }
        }
        // Clear bottom row
        int last_line = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
        for (int i = 0; i < VGA_WIDTH; i++) {
            video[last_line + i * 2]     = ' ';
            video[last_line + i * 2 + 1] = current_color;
        }
        cursor_row = VGA_HEIGHT - 1;
    }
}

void vga_putchar(char c) {
    volatile char *video = (volatile char *)VGA_ADDRESS;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        vga_scroll();
        vga_update_cursor(cursor_row, cursor_col);
        return;
    }

    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            int offset = (cursor_row * VGA_WIDTH + cursor_col) * 2;
            video[offset]     = ' ';
            video[offset + 1] = current_color;
        }
        vga_update_cursor(cursor_row, cursor_col);
        return;
    }

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
        vga_scroll();
    }

    int offset = (cursor_row * VGA_WIDTH + cursor_col) * 2;
    video[offset]     = c;
    video[offset + 1] = current_color;
    cursor_col++;
    vga_update_cursor(cursor_row, cursor_col);
}

void vga_print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

void vga_print_color(const char *str, uint8_t color) {
    uint8_t old = current_color;
    current_color = color;
    vga_print(str);
    current_color = old;
}

void vga_init(void) {
    vga_enable_cursor(14, 15);
    vga_clear();
}
