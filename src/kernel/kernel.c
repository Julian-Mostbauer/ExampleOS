// kernel.c - 32-bit Protected Mode Kernel in C

// The VGA text buffer is memory-mapped at physical address 0xB8000.
// Screen resolution: 80 columns x 25 rows.
// Each cell takes 2 bytes: [ASCII character] [Color attribute]
#define VGA_ADDRESS 0xB8000
#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A

void clear_screen(void) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    for (int i = 0; i < 80 * 25; i++) {
        video[i * 2]     = ' ';
        video[i * 2 + 1] = WHITE_ON_BLACK;
    }
}

void print_at(const char *str, int row, int col, char color) {
    volatile char *video = (volatile char *)VGA_ADDRESS;
    int offset = (row * 80 + col) * 2;

    for (int i = 0; str[i] != '\0'; i++) {
        video[offset]     = str[i];
        video[offset + 1] = color;
        offset += 2;
    }
}

void kmain(void) {
    clear_screen();

    print_at("==============================================", 2, 16, WHITE_ON_BLACK);
    print_at("Welcome to your custom 32-bit C Kernel!",        3, 18, GREEN_ON_BLACK);
    print_at("==============================================", 4, 16, WHITE_ON_BLACK);

    print_at("Running in 32-bit Protected Mode.",              7, 24, WHITE_ON_BLACK);
    print_at("Loaded from disk by custom bootloader.",         8, 20, WHITE_ON_BLACK);

    // Keep CPU halted to save power
    while (1) {
        __asm__ volatile("hlt");
    }
}
