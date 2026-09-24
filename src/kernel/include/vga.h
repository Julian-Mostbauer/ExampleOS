#ifndef _VGA_H_
#define _VGA_H_

#include "types.h"

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

void vga_init(void);
void vga_clear(void);
void vga_clear_absolute(void);
void vga_putchar(char c);
void vga_print(const char *str);
void vga_print_color(const char *str, uint8_t color);
void vga_set_status_badge(const char *badge);
void vga_update_cursor(int row, int col);
void vga_enable_cursor(uint8_t start, uint8_t end);
void vga_disable_cursor(void);
void vga_draw_status_bar(void);

#endif
