#include "pong.h"
#include "vga.h"
#include "keyboard.h"

static void frame_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __asm__ volatile("nop");
    }
}

static short abs(const short x) {
    return (x < 0) ? -x : x;
}

// 3x5 pixel digit font for score rendering
static const uint8_t digit_bitmap[10][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
    {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
    {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
    {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
    {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
    {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
    {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
    {0x7, 0x1, 0x2, 0x4, 0x4}, // 7
    {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
    {0x7, 0x5, 0x7, 0x1, 0x7}  // 9
};

static void draw_digit(uint16_t x, uint16_t y, uint8_t num, uint8_t color, uint8_t scale) {
    if (num > 9) num = 9;
    for (uint8_t r = 0; r < 5; r++) {
        for (uint8_t c = 0; c < 3; c++) {
            if (digit_bitmap[num][r] & (0x4 >> c)) {
                vga_draw_rect(x + c * scale, y + r * scale, scale, scale, color);
            }
        }
    }
}

static void draw_net(void) {
    for (uint16_t y = 4; y < VGA_GFX_HEIGHT; y += 12) {
        vga_draw_rect(VGA_GFX_WIDTH / 2 - 1, y, 2, 6, 8); // Dark gray dashed line
    }
}

void pong(void) {
    // 1. Enter 320x200 256-color graphics mode
    vga_set_mode_13h();
    vga_clear_screen_color(0); // Pure black

    const int16_t PLAYER_WIDTH  = 6;
    const int16_t PLAYER_HEIGHT = 38;
    const int16_t PLAYER_SPEED  = 3;
    const int16_t BALL_SIZE     = 6;

    const int16_t P1_X = 12;
    const int16_t P2_X = VGA_GFX_WIDTH - P1_X - PLAYER_WIDTH;

    int16_t p1_y = (VGA_GFX_HEIGHT - PLAYER_HEIGHT) / 2;
    int16_t p2_y = p1_y;

    int16_t prev_p1_y = p1_y;
    int16_t prev_p2_y = p2_y;

    int16_t ball_x = (VGA_GFX_WIDTH - BALL_SIZE) / 2;
    int16_t ball_y = (VGA_GFX_HEIGHT - BALL_SIZE) / 2;

    int16_t prev_ball_x = ball_x;
    int16_t prev_ball_y = ball_y;

    int16_t ball_vel_x = -3;
    int16_t ball_vel_y = 1;

    uint8_t score_p1 = 0;
    uint8_t score_p2 = 0;
    uint8_t prev_score_p1 = 255;
    uint8_t prev_score_p2 = 255;

    // Draw initial court net
    draw_net();

    bool running = true;
    while (running) {
        // --- 1. Non-blocking input polling (Quit game) ---
        while (keyboard_has_char()) {
            char c = keyboard_getchar_async();
            if (c == 'q' || c == 27) { // 'q' or ESC
                running = false;
            }
        }

        // --- 2. Real-time key state query (Player movement) ---
        // Player 1: 'W' and 'S'
        if (keyboard_is_key_down(KEY_W) && p1_y >= PLAYER_SPEED) {
            p1_y -= PLAYER_SPEED;
        }
        if (keyboard_is_key_down(KEY_S) && p1_y + PLAYER_HEIGHT + PLAYER_SPEED <= VGA_GFX_HEIGHT) {
            p1_y += PLAYER_SPEED;
        }

        // Player 2: Arrow UP and DOWN (also supports 'I' and 'K')
        if ((keyboard_is_key_down(KEY_UP) || keyboard_is_key_down(KEY_I)) && p2_y >= PLAYER_SPEED) {
            p2_y -= PLAYER_SPEED;
        }
        if ((keyboard_is_key_down(KEY_DOWN) || keyboard_is_key_down(KEY_K)) && p2_y + PLAYER_HEIGHT + PLAYER_SPEED <= VGA_GFX_HEIGHT) {
            p2_y += PLAYER_SPEED;
        }

        // --- 3. Ball Physics & Movement ---
        ball_x += ball_vel_x;
        ball_y += ball_vel_y;

        // Top & bottom wall bounce
        if (ball_y <= 0) {
            ball_y = 0;
            ball_vel_y = -ball_vel_y;
        } else if (ball_y + BALL_SIZE >= VGA_GFX_HEIGHT) {
            ball_y = VGA_GFX_HEIGHT - BALL_SIZE;
            ball_vel_y = -ball_vel_y;
        }

        // Player 1 Paddle Collision (Left)
        if (ball_vel_x < 0) {
            if (ball_x <= P1_X + PLAYER_WIDTH && ball_x + BALL_SIZE >= P1_X) {
                if (ball_y + BALL_SIZE >= p1_y && ball_y <= p1_y + PLAYER_HEIGHT) {
                    ball_x = P1_X + PLAYER_WIDTH; // Push outside paddle
                    ball_vel_x = -ball_vel_x;

                    // Deflect ball_vel_y depending on impact point
                    int16_t paddle_center = p1_y + PLAYER_HEIGHT / 2;
                    int16_t hit_offset = (ball_y + BALL_SIZE / 2) - paddle_center;
                    ball_vel_y = hit_offset / 6;
                    if (ball_vel_y == 0) ball_vel_y = (hit_offset >= 0) ? 1 : -1;
                }
            }
        }

        // Player 2 Paddle Collision (Right)
        if (ball_vel_x > 0) {
            if (ball_x + BALL_SIZE >= P2_X && ball_x <= P2_X + PLAYER_WIDTH) {
                if (ball_y + BALL_SIZE >= p2_y && ball_y <= p2_y + PLAYER_HEIGHT) {
                    ball_x = P2_X - BALL_SIZE; // Push outside paddle
                    ball_vel_x = -ball_vel_x;

                    // Deflect ball_vel_y depending on impact point
                    int16_t paddle_center = p2_y + PLAYER_HEIGHT / 2;
                    int16_t hit_offset = (ball_y + BALL_SIZE / 2) - paddle_center;
                    ball_vel_y = hit_offset / 6;
                    if (ball_vel_y == 0) ball_vel_y = (hit_offset >= 0) ? 1 : -1;
                }
            }
        }

        // --- 4. Scoring & Reset ---
        // P1 missed (Ball passed left wall) -> P2 scores
        if (ball_x < 0) {
            score_p2++;
            // Erase old ball before recentering
            vga_draw_rect(prev_ball_x, prev_ball_y, BALL_SIZE, BALL_SIZE, 0);
            ball_x = (VGA_GFX_WIDTH - BALL_SIZE) / 2;
            ball_y = (VGA_GFX_HEIGHT - BALL_SIZE) / 2;
            prev_ball_x = ball_x;
            prev_ball_y = ball_y;
            ball_vel_x = -3; // Serve towards P1
            ball_vel_y = 1;
            draw_net();
        }
        // P2 missed (Ball passed right wall) -> P1 scores
        else if (ball_x + BALL_SIZE > VGA_GFX_WIDTH) {
            score_p1++;
            // Erase old ball before recentering
            vga_draw_rect(prev_ball_x, prev_ball_y, BALL_SIZE, BALL_SIZE, 0);
            ball_x = (VGA_GFX_WIDTH - BALL_SIZE) / 2;
            ball_y = (VGA_GFX_HEIGHT - BALL_SIZE) / 2;
            prev_ball_x = ball_x;
            prev_ball_y = ball_y;
            ball_vel_x = 3;  // Serve towards P2
            ball_vel_y = -1;
            draw_net();
        }

        // --- 5. Flicker-Free Differential Rendering ---
        // Erase old paddle rects with black
        if (prev_p1_y != p1_y) {
            vga_draw_rect(P1_X, prev_p1_y, PLAYER_WIDTH, PLAYER_HEIGHT, 0);
        }
        if (prev_p2_y != p2_y) {
            vga_draw_rect(P2_X, prev_p2_y, PLAYER_WIDTH, PLAYER_HEIGHT, 0);
        }
        // Erase old ball with black
        vga_draw_rect(prev_ball_x, prev_ball_y, BALL_SIZE, BALL_SIZE, 0);

        // Redraw center dashed net where ball passed through
        if (abs(prev_ball_x - (VGA_GFX_WIDTH / 2)) <= BALL_SIZE + 4) {
            draw_net();
        }

        // Draw new paddle rects (White, color 15)
        vga_draw_rect(P1_X, p1_y, PLAYER_WIDTH, PLAYER_HEIGHT, 15);
        vga_draw_rect(P2_X, p2_y, PLAYER_WIDTH, PLAYER_HEIGHT, 15);

        // Draw new ball (Yellow, color 14)
        vga_draw_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 14);

        // Render scores (P1 on left, P2 on right)
        if (score_p1 != prev_score_p1) {
            draw_digit(120, 10, prev_score_p1, 0, 3); // Erase old
            draw_digit(120, 10, score_p1, 15, 3);      // Draw new
            prev_score_p1 = score_p1;
        }
        if (score_p2 != prev_score_p2) {
            draw_digit(180, 10, prev_score_p2, 0, 3); // Erase old
            draw_digit(180, 10, score_p2, 15, 3);      // Draw new
            prev_score_p2 = score_p2;
        }

        prev_p1_y = p1_y;
        prev_p2_y = p2_y;
        prev_ball_x = ball_x;
        prev_ball_y = ball_y;

        // --- 6. Frame pacing ---
        frame_delay(4000000);
    }

    // Restore text mode 03h and font when exiting
    vga_set_mode_03h();
    vga_clear();
}
