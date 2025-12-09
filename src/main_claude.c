// ---------------------------------------------------------------------------
// Asteroids Clone for RP6502 - MINIMAL RAM VERSION
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "colors.h"
#include "usb_hid_keys.h"
#include "bitmap_graphics_db.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 360
#define HALF_WIDTH 320
#define HALF_HEIGHT 180

#define NUM_ROTATION_STEPS 64
#define MAX_ASTEROIDS 8
#define MAX_BULLETS 12
#define BULLET_LIFETIME 100
#define SHIP_ACCEL 64
#define SHIP_MAX_SPEED 512
#define SHIP_FRICTION 2
#define BULLET_SPEED 800
#define ASTEROID_SPEED_LARGE 150
#define ASTEROID_SPEED_MEDIUM 200
#define ASTEROID_SPEED_SMALL 250
#define ASTEROID_ROT_SPEED 1  // Slower rotation
#define INVULNERABLE_TIME 90

#define KEYBOARD_INPUT 0xFF10
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

uint16_t buffers[2];
uint8_t active_buffer = 0;

// CRITICAL: Use const to store in ROM, not RAM!
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

// CRITICAL: Use const for shapes stored in ROM
const int8_t ship_shape[4][2] = {
    {10, 0}, {-8, 6}, {-5, 0}, {-8, -6}
};

const int8_t flame_shape[3][2] = {
    {-5, 3}, {-12, 0}, {-5, -3}
};

const int8_t asteroid_large[12][2] = {
    {0, -16}, {10, -14}, {16, -6}, {14, 4}, {16, 12}, {6, 16},
    {-4, 14}, {-12, 16}, {-16, 8}, {-16, -2}, {-10, -12}, {-2, -16}
};

const int8_t asteroid_medium[10][2] = {
    {0, -10}, {7, -8}, {10, -2}, {8, 6}, {2, 10},
    {-6, 10}, {-10, 4}, {-10, -4}, {-6, -10}, {-2, -10}
};

const int8_t asteroid_small[8][2] = {
    {0, -6}, {5, -4}, {6, 2}, {2, 6},
    {-4, 6}, {-6, 2}, {-6, -2}, {-2, -6}
};

typedef struct {
    int32_t x, y;  // Use 32-bit to handle 640<<8 and 360<<8
    int32_t vx, vy;  // Also 32-bit to handle large velocities
    uint8_t angle;
    bool active;
} Ship;

typedef struct {
    int32_t x, y;  // Use 32-bit for positions
    int32_t vx, vy;  // 32-bit for velocities
    uint8_t lifetime;
    bool active;
} Bullet;

typedef struct {
    int32_t x, y;  // Use 32-bit for positions
    int32_t vx, vy;  // 32-bit for velocities
    uint8_t size, angle, rot_speed;
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
    int16_t rotated_x[12], rotated_y[12];
    
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
            // Slower rotation: 0 or 1 step per frame
            asteroids[i].rot_speed = rand() & ASTEROID_ROT_SPEED;
            asteroids[i].active = true;
            return;
        }
    }
}

void init_level(void) {
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
    }
    
    uint8_t num_asteroids = level + 1;
    if (num_asteroids > 4) num_asteroids = 4;
    
    for (uint8_t i = 0; i < num_asteroids; i++) {
        int32_t x, y;
        do {
            x = (int32_t)(rand() % SCREEN_WIDTH) << 8;
            y = (int32_t)(rand() % SCREEN_HEIGHT) << 8;
        } while (abs((int16_t)(x >> 8) - HALF_WIDTH) < 100 && 
                 abs((int16_t)(y >> 8) - HALF_HEIGHT) < 100);
        
        // Generate random velocity in fixed point
        // Range: -32 to +32 pixels per frame (after >> 8)
        int32_t vx = (int32_t)((rand() % ASTEROID_SPEED_LARGE) - (ASTEROID_SPEED_LARGE >> 1));
        int32_t vy = (int32_t)((rand() % ASTEROID_SPEED_LARGE) - (ASTEROID_SPEED_LARGE >> 1));
        
        spawn_asteroid(0, x, y, vx, vy);
    }
    invulnerable_timer = INVULNERABLE_TIME;
}

void fire_bullet(void) {
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = ship.x;
            bullets[i].y = ship.y;
            
            int16_t sx = sin_table[ship.angle];
            int16_t cx = cos_table[ship.angle];
            // BULLET_SPEED is 320 (unscaled pixels per frame)
            // cx/sx are scaled by 256
            // Result: (320 * 256) >> 8 = 320 (velocity in fixed point matching position scale)
            bullets[i].vx = ship.vx + (((int32_t)BULLET_SPEED * cx) >> 8);
            bullets[i].vy = ship.vy + (((int32_t)BULLET_SPEED * sx) >> 8);
            
            bullets[i].lifetime = BULLET_LIFETIME;
            bullets[i].active = true;
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
                
                if (asteroids[j].size == 0) score += 20;
                else if (asteroids[j].size == 1) score += 50;
                else score += 100;
                
                if (asteroids[j].size < 2) {
                    uint8_t new_size = asteroids[j].size + 1;
                    uint8_t speed = (new_size == 1) ? ASTEROID_SPEED_MEDIUM : ASTEROID_SPEED_SMALL;
                    
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
                
                if (lives == 0) {
                    game_over = true;
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
    
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            if (asteroids[i].size == 0) {
                draw_rotated_polygon(asteroid_large, 12,
                                   asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
            } else if (asteroids[i].size == 1) {
                draw_rotated_polygon(asteroid_medium, 10,
                                   asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
            } else {
                draw_rotated_polygon(asteroid_small, 8,
                                   asteroids[i].x, asteroids[i].y, asteroids[i].angle, buffer);
            }
        }
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

int main(void) {
    buffers[0] = 0x0000;
    buffers[1] = 0x7080;
    
    init_bitmap_graphics(0xFF00, buffers[0], 0, 4, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
    
    erase_buffer(buffers[0]);
    erase_buffer(buffers[1]);
    
    srand(12345);
    init_ship();
    init_level();
    
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
    while (waiting) {
        read_keyboard();
        if (key(KEY_SPACE)) waiting = false;
    }
    
    while (key(KEY_SPACE)) {
        read_keyboard();
    }
    
    bool running = true;
    bool fire_was_pressed = false;
    
    while (running) {
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
                // SHIP_ACCEL is unscaled (8)
                // cx/sx are scaled by 256
                // Result: (8 * 256) >> 8 = 8 pixels per frame^2 in fixed point
                ship.vx += ((int32_t)SHIP_ACCEL * cx) >> 8;
                ship.vy += ((int32_t)SHIP_ACCEL * sx) >> 8;
                
                if (ship.vx > SHIP_MAX_SPEED) ship.vx = SHIP_MAX_SPEED;
                if (ship.vx < -SHIP_MAX_SPEED) ship.vx = -SHIP_MAX_SPEED;
                if (ship.vy > SHIP_MAX_SPEED) ship.vy = SHIP_MAX_SPEED;
                if (ship.vy < -SHIP_MAX_SPEED) ship.vy = -SHIP_MAX_SPEED;
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
        
        erase_buffer(buffers[!active_buffer]);
        draw_game(buffers[!active_buffer]);
        
        switch_buffer(buffers[!active_buffer]);
        active_buffer = !active_buffer;
    }
    
    return 0;
}