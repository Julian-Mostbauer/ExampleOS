#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"

void shell_run(void)
{
    char input[64];

    vga_print_color("Welcome to SimpleOS!\n", MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
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
            vga_print("  echo           - Repeat greeting message\n");
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
            vga_print_color("SimpleOS v0.3 (Modular Architecture)\n", MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            vga_print("Architecture: x86 32-bit Protected Mode\n");
            vga_print("Subsystems: CPU (IDT, PIC), Drivers (VGA, Keyboard), Lib, Shell\n");
        }
        else if (strcmp(input, "echo") == 0)
        {
            vga_print("Echo: Hallo aus dem modular strukturierten Kernel!\n");
        }
        else if (strcmp(input, "shutdown") == 0)
        {
            vga_clear_absolute();
            break;
        }
        else
        {
            vga_print_color("Unknown command: '", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
            vga_print_color(input, MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
            vga_print_color("'. Type 'help' for commands.\n", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
        }
    }
}
