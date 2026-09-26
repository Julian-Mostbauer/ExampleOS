#include "pong.h"
#include "vga.h"
#include "keyboard.h"

static void frame_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __asm__ volatile("nop");
    }
}

void pong(void) {
    vga_set_mode_13h();
    vga_clear_screen_color(1);

    const uint16_t PLAYER_WIDTH = 10;
    const uint16_t PLAYER_HEIGHT = 50;
    const uint16_t PLAYER_SPEED = 5;
    const uint16_t BALL_RAD = 12;
    const uint16_t P1_X = 10;
    const uint16_t P2_X = VGA_GFX_WIDTH - P1_X - PLAYER_WIDTH;

    uint16_t p1_y = (VGA_GFX_HEIGHT - PLAYER_HEIGHT) / 2;
    uint16_t p2_y = p1_y;

    uint16_t ball_x = 160 - BALL_RAD / 2;
    uint16_t ball_y = 160 - BALL_RAD / 2;

    bool running = true;
    while (running) {
        while (keyboard_has_char()) {
            const char c = keyboard_getchar_async();
            if (c == 'q' || c == 27) {
                running = false;
            }
        }

        // P1
        if (keyboard_is_key_down(KEY_W) && p1_y >= PLAYER_SPEED) p1_y -= PLAYER_SPEED;

        if (keyboard_is_key_down(KEY_S) && p1_y + PLAYER_HEIGHT + PLAYER_SPEED < VGA_GFX_HEIGHT)
            p1_y += PLAYER_SPEED;


        // P2
        if (keyboard_is_key_down(KEY_UP) && p2_y >= PLAYER_SPEED) p2_y -= PLAYER_SPEED;

        if (keyboard_is_key_down(KEY_DOWN) && p2_y + PLAYER_HEIGHT + PLAYER_SPEED < VGA_GFX_HEIGHT)
            p2_y += PLAYER_SPEED;


        vga_clear_screen_color(0);
        vga_draw_rect(P1_X, p1_y, PLAYER_WIDTH, PLAYER_HEIGHT, 15);
        vga_draw_rect(P2_X, p2_y, PLAYER_WIDTH, PLAYER_HEIGHT, 15);
        vga_draw_rect(ball_x - BALL_RAD / 2, ball_y - BALL_RAD / 2, BALL_RAD + 1, BALL_RAD + 1, 15);

        frame_delay(5000000);
    }

    // 5. Restore 80x25 text mode
    vga_set_mode_03h();
    vga_clear();
}
