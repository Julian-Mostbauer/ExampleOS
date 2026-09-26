#include "pong.h"
#include "vga.h"


void pong(void) {
    vga_set_mode_13h();
    vga_clear_screen_color(1);

    const uint16_t PLAYER_WIDTH = 10;
    const uint16_t PLAYER_HEIGHT = 50;
    const uint16_t BALL_RAD = 12;
    const uint16_t P1_X = 10;
    const uint16_t P2_X = VGA_GFX_WIDTH - P1_X - PLAYER_WIDTH;

    uint16_t p1_y = (VGA_GFX_HEIGHT - PLAYER_HEIGHT)/2;
    uint16_t p2_y = p1_y;

    uint16_t ball_x = 160 - BALL_RAD/2;
    uint16_t ball_y = 160 - BALL_RAD/2;

    while (1) {
        vga_draw_rect(P1_X, p1_y, PLAYER_WIDTH, PLAYER_HEIGHT, 15);
        vga_draw_rect(P2_X, p2_y, PLAYER_WIDTH, PLAYER_HEIGHT, 15);
        vga_draw_rect(ball_x - BALL_RAD/2, ball_y- BALL_RAD/2, BALL_RAD+1, BALL_RAD+1, 15);

    }

    // 5. Restore 80x25 text mode
    vga_set_mode_03h();
    vga_clear();
}