#include "vga.h"
#include "io.h"
#include "version.h"

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
    volatile char *video = (volatile char *)VGA_TEXT_ADDR;
    uint8_t bar_color   = MAKE_COLOR(COLOR_WHITE, COLOR_BLUE);
    uint8_t badge_color = MAKE_COLOR(COLOR_YELLOW, COLOR_BLUE);

    // Row 0 background
    for (int col = 0; col < VGA_WIDTH; col++) {
        video[col * 2]     = ' ';
        video[col * 2 + 1] = bar_color;
    }

    // Title on left
    const char *title = " " OS_NAME " 32-bit Kernel ";
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

// ------------------------------------------------------------
// VGA Mode Register Tables
// ------------------------------------------------------------
static const uint8_t mode_13h_regs[] = {
    // MISC (0x3C2)
    0x63,
    // SEQ (0x3C4 / 0x3C5, indices 0..4)
    0x03, 0x01, 0x0F, 0x00, 0x0E,
    // CRTC (0x3D4 / 0x3D5, indices 0..24)
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3, 0xFF,
    // GC (0x3CE / 0x3CF, indices 0..8)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF,
    // AC (0x3C0 / 0x3C1, indices 0..20)
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

static const uint8_t mode_03h_regs[] = {
    // MISC
    0x67,
    // SEQ
    0x03, 0x00, 0x03, 0x00, 0x02,
    // CRTC
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
    0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3, 0xFF,
    // GC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF,
    // AC
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x0C, 0x00, 0x0F, 0x08, 0x00
};

static void write_vga_registers(const uint8_t *regs) {
    // 1. Write MISC register
    outb(0x3C2, *regs++);

    // 2. Write Sequencer registers
    for (uint8_t i = 0; i < 5; i++) {
        outb(0x3C4, i);
        outb(0x3C5, *regs++);
    }

    // 3. Unlock CRTC registers 0..7 (clear protect bit 7 of register 0x11)
    outb(0x3D4, 0x11);
    outb(0x3D5, inb(0x3D5) & 0x7F);

    // Write CRTC registers
    for (uint8_t i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, *regs++);
    }

    // 4. Write Graphics Controller registers
    for (uint8_t i = 0; i < 9; i++) {
        outb(0x3CE, i);
        outb(0x3CF, *regs++);
    }

    // 5. Write Attribute Controller registers
    for (uint8_t i = 0; i < 21; i++) {
        inb(0x3DA); // Reset flip-flop
        outb(0x3C0, i);
        outb(0x3C0, *regs++);
    }

    // Enable video output (bit 5 = 1)
    inb(0x3DA);
    outb(0x3C0, 0x20);
}

void vga_set_mode_13h(void) {
    write_vga_registers(mode_13h_regs);
}

void vga_set_mode_03h(void) {
    write_vga_registers(mode_03h_regs);
}

void vga_put_pixel(uint16_t x, uint16_t y, uint8_t color) {
    if (x >= VGA_GFX_WIDTH || y >= VGA_GFX_HEIGHT) {
        return;
    }
    volatile uint8_t *video = (volatile uint8_t *)VGA_VIDEO_ADDR;
    video[VGA_GFX_WIDTH * y + x] = color;
}

void vga_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color) {
    for (uint16_t r = 0; r < h; r++) {
        for (uint16_t c = 0; c < w; c++) {
            vga_put_pixel(x + c, y + r, color);
        }
    }
}

void vga_clear_screen_color(uint8_t color) {
    volatile uint8_t *video = (volatile uint8_t *)VGA_VIDEO_ADDR;
    for (uint32_t i = 0; i < VGA_GFX_WIDTH * VGA_GFX_HEIGHT; i++) {
        video[i] = color;
    }
}

void vga_clear(void) {
    volatile char *video = (volatile char *)VGA_TEXT_ADDR;

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
    volatile char *video = (volatile char *)VGA_TEXT_ADDR;

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i * 2]     = ' ';
        video[i * 2 + 1] = current_color;
    }

    vga_disable_cursor();
}

static void vga_scroll(void) {
    volatile char *video = (volatile char *)VGA_TEXT_ADDR;
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
    volatile char *video = (volatile char *)VGA_TEXT_ADDR;

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
