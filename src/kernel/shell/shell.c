#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "version.h"

void shell_run(void)
{
    char input[64];

    vga_print_color("Welcome to " OS_NAME "!\n", MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    vga_print_color("Keyboard: German Standard (QWERTZ) active. Press F1 to toggle.\n", MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    vga_print_color("Type 'help' to see commands, or 'layout [de|us]' to change.\n\n", MAKE_COLOR(COLOR_LIGHT_GRAY, COLOR_BLACK));

    while (1)
    {
        vga_print_color("os> ", MAKE_COLOR(COLOR_GREEN, COLOR_BLACK));
        readline(input, sizeof(input));

        if (input[0] == '\0')
        {
            continue;
        }
        else if (strcmp(input, "help") == 0)
        {
            vga_print("Available commands:\n");
            vga_print("  layout [de|us] - View or switch keyboard layout (or press F1)\n");
            vga_print("  clear          - Clear the screen\n");
            vga_print("  about          - Display system information\n");
            vga_print("  echo           - prints given message\n");
            vga_print("  paint          - Switch to 320x200 graphics mode and paint pixels\n");
            vga_print("  shutdown       - Ends the running os\n");
            vga_print("  help           - Show this help menu\n");
        }
        else if (strcmp(input, "layout de") == 0 || strcmp(input, "de") == 0)
        {
            keyboard_set_layout(LAYOUT_DE);
            vga_print_color("Keyboard layout switched to German (DE - QWERTZ).\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
        }
        else if (strcmp(input, "layout us") == 0 || strcmp(input, "us") == 0)
        {
            keyboard_set_layout(LAYOUT_US);
            vga_print_color("Keyboard layout switched to US (QWERTY).\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
        }
        else if (strcmp(input, "layout") == 0)
        {
            vga_print_color("Current layout: ", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            vga_print_color(keyboard_get_layout_name(), MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            vga_print("\nUsage: 'layout de' or 'layout us' (or press F1 anytime).\n");
        }
        else if (strcmp(input, "clear") == 0)
        {
            vga_clear();
        }
        else if (strcmp(input, "about") == 0)
        {
            vga_print_color(OS_NAME " v" OS_VERSION "\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            vga_print("Architecture: x86 32-bit Protected Mode\n");
            vga_print("Subsystems: CPU (IDT, PIC), Drivers (VGA, Keyboard), Lib, Shell\n");
        }
        else if (strncmp(input, "echo", 4) == 0)
        {
            char *msg = input;
            msg += 5;
            vga_print(msg);
            vga_print("\n");
        }
        else if (strcmp(input, "shutdown") == 0)
        {
            vga_clear_absolute();
            break;
        }
        else if (strcmp(input, "paint") == 0 || strcmp(input, "graphics") == 0)
        {
            // 1. Switch to 320x200 256-color graphics mode
            vga_set_mode_13h();

            // 2. Clear background to dark blue (color 1)
            vga_clear_screen_color(1);

            // 3. Draw a 256-color palette gradient bar (using vga_put_pixel!)
            for (uint16_t x = 0; x < 256; x++) {
                for (uint16_t y = 15; y < 35; y++) {
                    vga_put_pixel(32 + x, y, (uint8_t)x);
                }
            }

            // 4. Draw demo rectangles (using vga_draw_rect)
            vga_draw_rect(32, 50, 50, 40, 4);   // Red
            vga_draw_rect(92, 50, 50, 40, 2);   // Green
            vga_draw_rect(152, 50, 50, 40, 14); // Yellow
            vga_draw_rect(212, 50, 50, 40, 5);  // Magenta

            // White frame & inner cyan box in the center
            vga_draw_rect(80, 110, 160, 60, 15);
            vga_draw_rect(85, 115, 150, 50, 3);

            // Wait for user keypress to return to text mode
            getchar();

            // 5. Restore 80x25 text mode
            vga_set_mode_03h();
            vga_clear();
        }
        else
        {
            vga_print_color("Unknown command: '", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
            vga_print_color(input, MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            vga_print_color("'. Type 'help' for commands.\n", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
        }
    }
}
