// ---------------------------------------------------------------------------
// Asteroids Clone for RP6502 - MINIMAL RAM VERSION WITH SOUND
// ---------------------------------------------------------------------------

// Optimize for size to fit in Release builds
#pragma GCC optimize ("Os")

#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "colors.h"
#include "usb_hid_keys.h"
#include "bitmap_graphics_db.h"
#include "ezpsg.h"
#include "sound.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 360
#define HALF_WIDTH 320
#define HALF_HEIGHT 180

#define NUM_ROTATION_STEPS 64
#define MAX_ASTEROIDS 8
#define MAX_BULLETS 12
#define BULLET_LIFETIME 80
#define SHIP_ACCEL 64
#define SHIP_MAX_SPEED 512
#define SHIP_FRICTION 2
#define BULLET_SPEED 1800
#define ASTEROID_SPEED_LARGE 500
#define ASTEROID_SPEED_MEDIUM 700
#define ASTEROID_SPEED_SMALL 900
#define ASTEROID_ROT_SPEED 1
#define INVULNERABLE_TIME 90

#define NUM_LARGE_SHAPES 3
#define NUM_MEDIUM_SHAPES 3
#define NUM_SMALL_SHAPES 3

#define KEYBOARD_INPUT 0xFF10
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

uint16_t buffers[2];
uint8_t active_buffer = 0;

const int16_t sin_table[NUM_ROTATION_STEPS] = {
    0, 25, 50, 74, 98, 120, 142, 162, 180, 197, 212, 225, 236, 244, 250, 254,
    256, 254, 250, 244, 236, 225, 212, 197, 180, 162, 142, 120, 98, 74, 50, 25,
    0, -25, -50, -74, -98, -120, -142, -162, -180, -197, -212, -225, -236, -244, -250, -254,
    -256, -254, -250, -244, -236, -225, -212, -197, -180, -162, -142, -120, -98, -74, -50, -25
};

const int16_t cos_table[NUM_ROTATION_STEPS] = {
    256, 254, 250, 244, 236, 225, 212, 197, 180, 162, 142, 120, 98, 74, 50, 25,
    0, -25, -50, -74, -98, -120, -142, -162, -180, -197, -212, -225, -236, -244, -250, -254,
    -256, -254, -250, -244, -236, -225, -212, -197, -180, -162, -142, -120, -98, -74, -50, -25,
    0, 25, 50, 74, 98, 120, 142, 162, 180, 197, 212, 225, 236, 244, 250, 254
};

const int8_t ship_shape[4][2] = {
    {10, 0}, {-8, 6}, {-5, 0}, {-8, -6}
};

const int8_t flame_shape[3][2] = {
    {-5, 3}, {-12, 0}, {-5, -3}
};

// Large asteroid shapes (4 variations)
const int8_t asteroid_large_1[12][2] = {
    {7, -15}, {-4, -11}, {-7, -15}, {-15, -7}, {-11, 0}, {-15, 8},
    {-6, 15}, {0, 11}, {8, 15}, {16, 7}, {7, 4}, {15, -4}
};

const int8_t asteroid_large_2[12][2] = {
    {5, 15}, {16, 8}, {16, 4}, {4, 0}, {15, -7}, {9, -15},
    {3, -11}, {-7, -15}, {-14, -3}, {-14, 8}, {-2, 7}, {-6, 15}
};

const int8_t asteroid_large_3[10][2] = {
    {1, 8}, {8, 15}, {16, 9}, {11, 1}, {16, -7}, {5, -15},
    {-7, -15}, {-14, -9}, {-14, 8}, {-6, 15}
};

// Medium asteroid shapes (4 variations)
const int8_t asteroid_medium_1[12][2] = {
    {4, -8}, {-2, -6}, {-4, -8}, {-7, -4}, {-5, -1}, {-7, 4},
    {-3, 7}, {0, 5}, {4, 7}, {8, 3}, {4, 1}, {8, -2}
};

const int8_t asteroid_medium_2[12][2] = {
    {8, 2}, {2, 0}, {8, -4}, {5, -8}, {1, -5}, {-4, -8},
    {-7, -2}, {-7, 4}, {-1, 3}, {-3, 7}, {3, 7}, {8, 4}
};

const int8_t asteroid_medium_3[10][2] = {
    {8, 4}, {5, 0}, {8, -4}, {3, -8}, {-3, -8}, {-7, -5},
    {-7, 4}, {-3, 7}, {0, 4}, {4, 7}
};

// Small asteroid shapes (4 variations)
const int8_t asteroid_small_1[12][2] = {
    {2, -3}, {-1, -2}, {-2, -3}, {-3, -2}, {-2, 0}, {-3, 2},
    {-2, 3}, {0, 2}, {1, 3}, {3, 2}, {2, 0}, {3, -1}
};

const int8_t asteroid_small_2[12][2] = {
    {2, 4}, {4, 2}, {4, 1}, {1, 0}, {4, -2}, {3, -4},
    {1, -3}, {-1, -4}, {-3, -1}, {-3, 2}, {0, 2}, {-1, 4}
};

const int8_t asteroid_small_3[10][2] = {
    {3, 0}, {4, -2}, {2, -4}, {-1, -4}, {-3, -2}, {-3, 2},
    {-1, 4}, {0, 2}, {2, 4}, {4, 2}
};

typedef struct {
    int32_t x, y;
    int32_t vx, vy;
    uint8_t angle;
    bool active;
} Ship;

typedef struct {
    int32_t x, y;
    int32_t vx, vy;
    uint8_t lifetime;
    bool active;
} Bullet;

typedef struct {
    int32_t x, y;
    int32_t vx, vy;
    uint8_t size, angle, rot_speed;
    uint8_t shape_index;  // New: which shape variation to use
    bool active;
} Asteroid;

Ship ship;
Bullet bullets[MAX_BULLETS];
Asteroid asteroids[MAX_ASTEROIDS];

uint16_t score = 0;
uint8_t lives = 3;
uint8_t level = 1;
uint8_t invulnerable_timer = 0;
bool game_over = false;
bool thrust_on = false;
uint8_t frame_counter = 0;

char text_buffer[24];

void wrap_position(int32_t *x, int32_t *y) {
    int32_t sw = (int32_t)SCREEN_WIDTH << 8;
    int32_t sh = (int32_t)SCREEN_HEIGHT << 8;
    
    if (*x < 0) *x += sw;
    if (*x >= sw) *x -= sw;
    if (*y < 0) *y += sh;
    if (*y >= sh) *y -= sh;
}

void draw_rotated_polygon(const int8_t vertices[][2], uint8_t num_verts, 
                          int32_t x, int32_t y, uint8_t angle, uint16_t buffer) {
    int16_t rotated_x[13], rotated_y[13];
    
    int16_t sx = sin_table[angle];
    int16_t cx = cos_table[angle];
    
    for (uint8_t i = 0; i < num_verts; i++) {
        int8_t vx = vertices[i][0];
        int8_t vy = vertices[i][1];
        
        int16_t rx = ((int16_t)vx * cx - (int16_t)vy * sx) >> 8;
        int16_t ry = ((int16_t)vx * sx + (int16_t)vy * cx) >> 8;
        
        rotated_x[i] = (int16_t)(x >> 8) + rx;
        rotated_y[i] = (int16_t)(y >> 8) + ry;
    }
    
    for (uint8_t i = 0; i < num_verts; i++) {
        uint8_t next = (i + 1) % num_verts;
        draw_line2buffer(WHITE, rotated_x[i], rotated_y[i], 
                        rotated_x[next], rotated_y[next], buffer);
    }
}

void init_ship(void) {
    ship.x = (int32_t)HALF_WIDTH << 8;
    ship.y = (int32_t)HALF_HEIGHT << 8;
    ship.vx = 0;
    ship.vy = 0;
    ship.angle = 48;
    ship.active = true;
    invulnerable_timer = INVULNERABLE_TIME;
}

void spawn_asteroid(uint8_t size, int32_t x, int32_t y, int32_t vx, int32_t vy) {
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {
            asteroids[i].x = x;
            asteroids[i].y = y;
            asteroids[i].vx = vx;
            asteroids[i].vy = vy;
            asteroids[i].size = size;
            asteroids[i].angle = rand() & 63;
            asteroids[i].rot_speed = rand() & ASTEROID_ROT_SPEED;
            
            // Randomly select a shape variation based on size
            if (size == 0) {
                // asteroids[i].shape_index = 0;
                asteroids[i].shape_index = random(0, NUM_LARGE_SHAPES); 
            } else if (size == 1) {
                asteroids[i].shape_index = random(0, NUM_MEDIUM_SHAPES);
            } else {
                asteroids[i].shape_index = random(0, NUM_SMALL_SHAPES);
            }
            
            asteroids[i].active = true;
            printf("Spawning asteroid %u, size: %u, shape index: %u, x: %d, y: %d\n", i, size, asteroids[i].shape_index, (int)(x >> 8), (int)(y >> 8));
            return;
        }
    }
}

void init_level(void) {
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
    }

    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].active = false;
        }
    }
    
    uint8_t num_asteroids = level + 1;
    if (num_asteroids > 4) num_asteroids = 4;
    
    printf("Level %u: Spawning %u asteroids\n", level, num_asteroids);
    
    for (uint8_t i = 0; i < num_asteroids; i++) {
        int32_t x, y;
        // do {
            x = (int32_t)random(0, SCREEN_WIDTH) << 8;
            // x = (int32_t)((uint16_t)rand() % SCREEN_WIDTH) << 8;
            y = (int32_t)random(0, SCREEN_HEIGHT) << 8;
            // y = (int32_t)((uint16_t)rand() % SCREEN_HEIGHT) << 8;
        // } while (abs((int16_t)(x >> 8) - HALF_WIDTH) < 100 && 
        //          abs((int16_t)(y >> 8) - HALF_HEIGHT) < 100);
        
        int32_t vx = (int32_t)((rand() % ASTEROID_SPEED_LARGE) - (ASTEROID_SPEED_LARGE >> 1));
        int32_t vy = (int32_t)((rand() % ASTEROID_SPEED_LARGE) - (ASTEROID_SPEED_LARGE >> 1));
        
        spawn_asteroid(0, x, y, vx, vy);
        printf("Spawned asteroid %u at x=%d y=%d\n", i, (int)(x >> 8), (int)(y >> 8));
    }
    
    // Debug: Count active asteroids
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) count++;
    }
    printf("Active asteroids after spawn: %u\n", count);
    
    invulnerable_timer = INVULNERABLE_TIME;
}

void fire_bullet(void) {
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = ship.x;
            bullets[i].y = ship.y;
            
            int16_t sx = sin_table[ship.angle];
            int16_t cx = cos_table[ship.angle];
            bullets[i].vx = ship.vx + (((int32_t)BULLET_SPEED * cx) >> 8);
            bullets[i].vy = ship.vy + (((int32_t)BULLET_SPEED * sx) >> 8);
            
            bullets[i].lifetime = BULLET_LIFETIME;
            bullets[i].active = true;
            
            // SOUND: Play shoot sound
            play_shoot_sound();
            return;
        }
    }
}

bool check_collision(int32_t x1, int32_t y1, uint8_t r1,
                     int32_t x2, int32_t y2, uint8_t r2) {
    int16_t dx = (int16_t)(x1 >> 8) - (int16_t)(x2 >> 8);
    int16_t dy = (int16_t)(y1 >> 8) - (int16_t)(y2 >> 8);
    uint8_t sum_r = r1 + r2;
    
    if (abs(dx) > sum_r || abs(dy) > sum_r) return false;
    
    int16_t dist_sq = (dx * dx) + (dy * dy);
    int16_t r_sq = sum_r * sum_r;
    
    return dist_sq < r_sq;
}

void update_game(void) {
    if (game_over) return;
    
    frame_counter++;
    if (invulnerable_timer > 0) invulnerable_timer--;
    
    if (ship.active) {
        if (ship.vx > 0) ship.vx -= SHIP_FRICTION;
        else if (ship.vx < 0) ship.vx += SHIP_FRICTION;
        if (ship.vy > 0) ship.vy -= SHIP_FRICTION;
        else if (ship.vy < 0) ship.vy += SHIP_FRICTION;
        
        ship.x += ship.vx;
        ship.y += ship.vy;
        wrap_position(&ship.x, &ship.y);
    }
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].vx;
            bullets[i].y += bullets[i].vy;
            wrap_position(&bullets[i].x, &bullets[i].y);
            
            bullets[i].lifetime--;
            if (bullets[i].lifetime == 0) {
                bullets[i].active = false;
            }
        }
    }
    
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            asteroids[i].x += asteroids[i].vx;
            asteroids[i].y += asteroids[i].vy;
            wrap_position(&asteroids[i].x, &asteroids[i].y);
            
            asteroids[i].angle = (asteroids[i].angle + asteroids[i].rot_speed) & 63;
        }
    }
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        
        for (uint8_t j = 0; j < MAX_ASTEROIDS; j++) {
            if (!asteroids[j].active) continue;
            
            uint8_t asteroid_radius = (asteroids[j].size == 0) ? 16 : 
                                     (asteroids[j].size == 1) ? 10 : 6;
            
            if (check_collision(bullets[i].x, bullets[i].y, 1,
                               asteroids[j].x, asteroids[j].y, asteroid_radius)) {
                bullets[i].active = false;
                asteroids[j].active = false;
                
                // SOUND: Play explosion based on asteroid size
                play_explosion_sound(asteroids[j].size);
                
                if (asteroids[j].size == 0) score += 20;
                else if (asteroids[j].size == 1) score += 50;
                else score += 100;
                
                if (asteroids[j].size < 2) {
                    uint8_t new_size = asteroids[j].size + 1;
                    uint16_t speed = (new_size == 1) ? ASTEROID_SPEED_MEDIUM : ASTEROID_SPEED_SMALL;
                    
                    int16_t vx1 = ((rand() % speed) - (speed >> 1));
                    int16_t vy1 = ((rand() % speed) - (speed >> 1));
                    spawn_asteroid(new_size, asteroids[j].x, asteroids[j].y, vx1, vy1);
                    
                    int16_t vx2 = ((rand() % speed) - (speed >> 1));
                    int16_t vy2 = ((rand() % speed) - (speed >> 1));
                    spawn_asteroid(new_size, asteroids[j].x, asteroids[j].y, vx2, vy2);
                }
                break;
            }
        }
    }
    
    if (ship.active && invulnerable_timer == 0) {
        for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
            if (!asteroids[i].active) continue;
            
            uint8_t asteroid_radius = (asteroids[i].size == 0) ? 16 : 
                                     (asteroids[i].size == 1) ? 10 : 6;
            
            if (check_collision(ship.x, ship.y, 8,
                               asteroids[i].x, asteroids[i].y, asteroid_radius)) {
                ship.active = false;
                lives--;
                
                // SOUND: Play large explosion for ship
                play_explosion_sound(0);
                
                if (lives == 0) {
                    game_over = true;
                    play_game_over_sound();
                } else {
                    init_ship();
                }
                break;
            }
        }
    }
    
    bool any_asteroids = false;
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            any_asteroids = true;
            break;
        }
    }
    
    if (!any_asteroids) {
        level++;
        init_level();
    }
}

void draw_game(uint16_t buffer) {
    if (ship.active && (invulnerable_timer == 0 || (frame_counter & 4))) {
        draw_rotated_polygon(ship_shape, 4, ship.x, ship.y, ship.angle, buffer);
        
        if (thrust_on && (frame_counter & 2)) {
            draw_rotated_polygon(flame_shape, 3, ship.x, ship.y, ship.angle, buffer);
        }
    }
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            int16_t bx = (int16_t)(bullets[i].x >> 8);
            int16_t by = (int16_t)(bullets[i].y >> 8);
            draw_pixel2buffer(WHITE, bx, by, buffer);
            draw_pixel2buffer(WHITE, bx + 1, by, buffer);
            draw_pixel2buffer(WHITE, bx, by + 1, buffer);
            draw_pixel2buffer(WHITE, bx + 1, by + 1, buffer);
        }
    }
    
    // Debug: count and draw asteroids
    uint8_t drawn_count = 0;
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            drawn_count++;
            int16_t ax = (int16_t)(asteroids[i].x >> 8);
            int16_t ay = (int16_t)(asteroids[i].y >> 8);
            
            if (asteroids[i].size == 0) {
                // Large asteroids - 4 variations
                switch (asteroids[i].shape_index) {
                    case 0:
                        draw_rotated_polygon(asteroid_large_1, 12,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                    case 1:
                        draw_rotated_polygon(asteroid_large_2, 12,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                    case 2:
                        draw_rotated_polygon(asteroid_large_3, 10,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                }
            } else if (asteroids[i].size == 1) {
                // Medium asteroids - 4 variations
                switch (asteroids[i].shape_index) {
                    case 0:
                        draw_rotated_polygon(asteroid_medium_1, 12,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                    case 1:
                        draw_rotated_polygon(asteroid_medium_2, 12,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                    case 2:
                        draw_rotated_polygon(asteroid_medium_3, 10,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                }
            } else {
                // Small asteroids - 4 variations
                switch (asteroids[i].shape_index) {
                    case 0:
                        draw_rotated_polygon(asteroid_small_1, 12,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                    case 1:
                        draw_rotated_polygon(asteroid_small_2, 12,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                    case 2:
                        draw_rotated_polygon(asteroid_small_3, 10,
                                           asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
                        break;
                }
            }
        }
    }
    
    if (drawn_count > 0 && (frame_counter % 60) == 0) {
        printf("Drew %u asteroids this frame\n", drawn_count);
    }
    
    set_cursor(10, 10);
    sprintf(text_buffer, "SCR:%u", score);
    draw_string2buffer(text_buffer, buffer);
    
    set_cursor(10, 25);
    sprintf(text_buffer, "LVS:%u LVL:%u", lives, level);
    draw_string2buffer(text_buffer, buffer);
    
    if (game_over) {
        set_cursor(260, 170);
        draw_string2buffer("GAME OVER", buffer);
        set_cursor(240, 190);
        draw_string2buffer("Press SPACE", buffer);
    }
}

void read_keyboard(void) {
    xregn(0, 0, 0, 1, KEYBOARD_INPUT);
    for (uint8_t i = 0; i < KEYBOARD_BYTES; i++) {
        RIA.addr0 = KEYBOARD_INPUT + i;
        keystates[i] = RIA.rw0;
    }
}

/* quick test - call from main after init_sound() */
void test_waveforms(void) {
    printf("test_waveforms\n");
    // Square tone (should be audible)
    uint16_t x = ezpsg_play_note(
        c6,    // note
        30,    // duration
        30,    // release
        0xFF,  // duty
        0xF0,  // vol_attack
        0x90,  // vol_decay
        EZPSG_WAVE_SQUARE,
        EZPSG_PAN_CENTER
    );
    if (x != 0xFFFF) {
        RIA.addr0 = x + 5;   // wave_release is the 6th byte in struct (0..5): freq lo, hi, duty, vol_attack, vol_decay, wave_release
        RIA.step0 = 0;
        uint8_t val = RIA.rw0;
        printf("square val: %i\n", val);
        printf("psg base = %04x\n", x);
    }

    // small delay (let the tick call advance a bit)
    for (int i = 0; i < 60; ++i) ezpsg_tick(1);

    // Noise (should be audible if waveform defined correctly)
    x = ezpsg_play_note(
        c6,
        30,
        30,
        0xFF,
        0xF0,
        0x90,
        EZPSG_WAVE_NOISE,
        EZPSG_PAN_CENTER
    );
    if (x != 0xFFFF) {
        RIA.addr0 = x + 5;   // wave_release is the 6th byte in struct (0..5): freq lo, hi, duty, vol_attack, vol_decay, wave_release
        RIA.step0 = 0;
        uint8_t val = RIA.rw0;
        printf("noise val: %i\n", val);
        printf("psg base = %04x\n", x);
    }
}


int main(void) {
    buffers[0] = 0x0000;
    buffers[1] = 0x7080;
    
    init_bitmap_graphics(0xFF00, buffers[0], 0, 4, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
    
    // SOUND: Initialize sound system
    init_sound();
    // test_waveforms();
    
    erase_buffer(buffers[0]);
    erase_buffer(buffers[1]);
    
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
    
    active_buffer = 0;
    switch_buffer(buffers[active_buffer]);
    
    set_cursor(220, 140);
    draw_string2buffer("*** ASTEROIDS ***", buffers[active_buffer]);
    set_cursor(180, 180);
    draw_string2buffer("Arrows:Rotate/Thrust", buffers[active_buffer]);
    set_cursor(180, 195);
    draw_string2buffer("Space:Fire ESC:Quit", buffers[active_buffer]);
    set_cursor(220, 220);
    draw_string2buffer("Press SPACE", buffers[active_buffer]);

    bool waiting = true;
    uint16_t seed = 0;
    while (waiting) {
        seed++;
        read_keyboard();
        if (key(KEY_SPACE)) waiting = false;
    }
    
    while (key(KEY_SPACE)) {
        read_keyboard();
    }

    srand(seed);
    init_ship();
    init_level();
    
    bool running = true;
    bool fire_was_pressed = false;

    uint8_t v; // vsync counter, incements every 1/60 second, rolls over every 256
    // vsync loop
    v = RIA.vsync;
    
    while (running) {
        if (v == RIA.vsync) {
            continue; // wait until vsync is incremented
        } else {
            v = RIA.vsync; // new value for v
        }

        read_keyboard();
        
        if (game_over) {
            if (key(KEY_SPACE)) {
                score = 0;
                lives = 3;
                level = 1;
                game_over = false;
                init_ship();
                init_level();
                for (uint8_t i = 0; i < MAX_BULLETS; i++) {
                    bullets[i].active = false;
                }
            }
        } else {
            if (key(KEY_LEFT)) {
                ship.angle = (ship.angle - 1) & 63;
            }
            if (key(KEY_RIGHT)) {
                ship.angle = (ship.angle + 1) & 63;
            }
            
            thrust_on = false;
            if (key(KEY_UP)) {
                thrust_on = true;
                int16_t sx = sin_table[ship.angle];
                int16_t cx = cos_table[ship.angle];
                ship.vx += ((int32_t)SHIP_ACCEL * cx) >> 8;
                ship.vy += ((int32_t)SHIP_ACCEL * sx) >> 8;
                
                if (ship.vx > SHIP_MAX_SPEED) ship.vx = SHIP_MAX_SPEED;
                if (ship.vx < -SHIP_MAX_SPEED) ship.vx = -SHIP_MAX_SPEED;
                if (ship.vy > SHIP_MAX_SPEED) ship.vy = SHIP_MAX_SPEED;
                if (ship.vy < -SHIP_MAX_SPEED) ship.vy = -SHIP_MAX_SPEED;
                
                // SOUND: Start the thrust sound (or keep it going)
                start_thrust_sound();
            } else {
                // SOUND: Tell our system the thrust sound should stop.
                // The sound will fade out on its own.
                stop_thrust_sound();
            }
            
            if (key(KEY_SPACE)) {
                if (!fire_was_pressed) {
                    fire_bullet();
                    fire_was_pressed = true;
                }
            } else {
                fire_was_pressed = false;
            }
        }
        
        if (key(KEY_ESC)) {
            running = false;
        }
        
        update_game();
        
        // SOUND: Update sound effects each frame
        update_sound();
        
        erase_buffer(buffers[!active_buffer]);
        draw_game(buffers[!active_buffer]);
        
        switch_buffer(buffers[!active_buffer]);
        active_buffer = !active_buffer;
    }
    
    return 0;
}