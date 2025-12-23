// ---------------------------------------------------------------------------
// Asteroids Clone for RP6502 - MINIMAL RAM VERSION WITH SOUND
// ---------------------------------------------------------------------------

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
#define MAX_BULLETS 8
#define BULLET_LIFETIME 40
#define MAX_PARTICLES 16
#define PARTICLE_LIFETIME 10
#define SHIP_ACCEL 64
#define SHIP_MAX_SPEED 512
#define SHIP_FRICTION 2
#define BULLET_SPEED 1800
#define ASTEROID_SPEED_LARGE 500
#define ASTEROID_SPEED_MEDIUM 700
#define ASTEROID_SPEED_SMALL 900
#define ASTEROID_ROT_SPEED 1
#define INVULNERABLE_TIME 90
#define UFO_SPAWN_TIME 500
#define UFO_SPEED 800
#define UFO_SHOOT_INTERVAL 40


#define NUM_LARGE_SHAPES 3
#define NUM_MEDIUM_SHAPES 3
#define NUM_SMALL_SHAPES 3

#define KEYBOARD_INPUT 0xFF10
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

uint8_t scale = 80;

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

const int8_t ufo_shape[12][2] = {
    {-4, 3}, {-11, -1}, {-4, -5}, {4, -5}, {11, -1}, {4, 3},
    {3, 7}, {-3, 7}, {-4, 3}, {4, 3}, {11, -1}, {-11, -1}
};

typedef struct { int32_t x, y, vx, vy; 
    uint8_t shoot_timer; 
    bool active; } UFO;

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

typedef struct { 
    int32_t x, y, vx, vy; 
    uint8_t lifetime; 
    bool active; 
} Particle;

#define MAX_SHIP_DEBRIS 4
typedef struct {
    int32_t x, y;        // Center position of the line
    int32_t vx, vy;      // Velocity
    uint8_t angle;       // Rotation of the line itself
    int8_t rot_speed;    // How fast the line spins
    uint8_t lifetime;    // Fade out timer
    bool active;
} DebrisLine;

DebrisLine ship_debris[MAX_SHIP_DEBRIS];


Ship ship;
Bullet bullets[MAX_BULLETS];
Bullet ufo_bullets[MAX_BULLETS];
Asteroid asteroids[MAX_ASTEROIDS];
Particle particles[MAX_PARTICLES];
UFO ufo;


uint16_t score = 0;
uint8_t lives = 3;
uint8_t level = 1;
uint8_t invulnerable_timer = 0;
bool game_over = false;
bool thrust_on = false;
uint8_t frame_counter = 0;
uint16_t ufo_spawn_timer = 0;
int8_t pan;

char text_buffer[24];

InterpSoundHandle ufo_sound;

// Pre-calculated scaled rotation coefficients
int16_t scaled_sin[NUM_ROTATION_STEPS];
int16_t scaled_cos[NUM_ROTATION_STEPS];

void precalculate_rotation_tables() {
    for (uint8_t angle = 0; angle < NUM_ROTATION_STEPS; angle++) {
        scaled_sin[angle] = (sin_table[angle] * scale) / 100;
        scaled_cos[angle] = (cos_table[angle] * scale) / 100;
    }
}

/**
 * @brief Converts a 24.8 fixed-point value to a scaled int8_t.
 * 
 * @param fixed_pos_24_8 The input value in 24.8 fixed-point format.
 *                       The integer part is assumed to be in the range [0, 640].
 * @return An int8_t value mapped to the range [-63, 63].
 */
int8_t convert_pos_to_pan(int32_t fixed_pos_24_8) {
    int16_t integer_pos = (int16_t)(fixed_pos_24_8 >> 8);
    if (integer_pos < 0)   integer_pos = 0;
    if (integer_pos > 640) integer_pos = 640;
    int32_t scaled_val = ((int32_t)integer_pos * 63) / 320;
    int8_t final_val = (int8_t)(scaled_val - 63);

    return final_val;
}

void spawn_ship_shatter(void) {
    for (uint8_t i = 0; i < MAX_SHIP_DEBRIS; i++) {
        ship_debris[i].x = ship.x;
        ship_debris[i].y = ship.y;
        
        ship_debris[i].vx = (ship.vx / 2) + ((rand() % 400) - 200);
        ship_debris[i].vy = (ship.vy / 2) + ((rand() % 400) - 200);
        
        ship_debris[i].angle = ship.angle;
        ship_debris[i].rot_speed = (rand() % 5) - 2; 
        ship_debris[i].lifetime = 30; 
        ship_debris[i].active = true;
    }
    ship.active = false; 
}

void spawn_particles(int32_t x, int32_t y, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        for (uint8_t j = 0; j < MAX_PARTICLES; j++) {
            if (!particles[j].active) {
                particles[j].x = x; particles[j].y = y;
                int16_t angle = rand() & 63, speed = 100 + (rand() % 700);
                particles[j].vx = ((int32_t)speed * cos_table[angle]) >> 8;
                particles[j].vy = ((int32_t)speed * sin_table[angle]) >> 8;
                particles[j].lifetime = PARTICLE_LIFETIME + (rand() % 10);
                particles[j].active = true;
                break;
            }
        }
    }
}

void draw_rotated_polygon(const int8_t vertices[][2], uint8_t num_verts, 
                          int32_t x, int32_t y, uint8_t angle, uint16_t buffer) {
    int16_t rotated_x[13], rotated_y[13];
    
    int16_t sx = scaled_sin[angle];
    int16_t cx = scaled_cos[angle];
    
    for (uint8_t i = 0; i < num_verts; i++) {
        int8_t vx = vertices[i][0];
        int8_t vy = vertices[i][1];
        
        int16_t rx = ((int16_t)vx * cx - (int16_t)vy * sx) >> 8;
        int16_t ry = ((int16_t)vx * sx + (int16_t)vy * cx) >> 8;
        
        rotated_x[i] = (int16_t)(x >> 8) + rx;
        rotated_y[i] = (int16_t)(y >> 8) + ry;
    }
    
    for (uint8_t i = 0; i < num_verts; i++) {
        uint8_t next = (i + 1 < num_verts) ? (i + 1) : 0;
        draw_line2buffer(WHITE, rotated_x[i], rotated_y[i], 
                        rotated_x[next], rotated_y[next], buffer);
    }
}

void wrap_position(int32_t *x, int32_t *y) {
    int32_t sw = (int32_t)SCREEN_WIDTH << 8;
    int32_t sh = (int32_t)SCREEN_HEIGHT << 8;
    
    if (*x < 0) *x += sw;
    if (*x >= sw) *x -= sw;
    if (*y < 0) *y += sh;
    if (*y >= sh) *y -= sh;
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
            
            if (size == 0) {
                asteroids[i].shape_index = random(0, NUM_LARGE_SHAPES); 
            } else if (size == 1) {
                asteroids[i].shape_index = random(0, NUM_MEDIUM_SHAPES);
            } else {
                asteroids[i].shape_index = random(0, NUM_SMALL_SHAPES);
            }
            
            asteroids[i].active = true;
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
    
    ufo.active = false; ufo_spawn_timer = UFO_SPAWN_TIME;
    stop_interpolated_sound(ufo_sound);

    uint8_t num_asteroids = level + 1;
    if (num_asteroids > 4) num_asteroids = 4;
    
    for (uint8_t i = 0; i < num_asteroids; i++) {
        int32_t x, y;
        x = (int32_t)random(0, SCREEN_WIDTH) << 8;
        y = (int32_t)random(0, SCREEN_HEIGHT) << 8;
        
        int32_t vx = (int32_t)((rand() % ASTEROID_SPEED_LARGE) - (ASTEROID_SPEED_LARGE >> 1));
        int32_t vy = (int32_t)((rand() % ASTEROID_SPEED_LARGE) - (ASTEROID_SPEED_LARGE >> 1));
        
        spawn_asteroid(0, x, y, vx, vy);
    }
    
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) count++;
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
            bullets[i].vx = ship.vx + (((int32_t)BULLET_SPEED * cx) >> 8);
            bullets[i].vy = ship.vy + (((int32_t)BULLET_SPEED * sx) >> 8);
            
            bullets[i].lifetime = BULLET_LIFETIME;
            bullets[i].active = true;
            
            pan = convert_pos_to_pan(ship.x);
            start_bullet_sound(pan);
            return;
        }
    }
}


void spawn_ufo(void) {
    if (ufo.active) return;
    ufo.x = (rand() & 1) ? 20 : ((int32_t)(SCREEN_WIDTH - 20) << 8);
    ufo.y = (int32_t)random(50, SCREEN_HEIGHT - 50) << 8;
    ufo.vx = (ufo.x == 20) ? UFO_SPEED : -UFO_SPEED; ufo.vy = ((rand() % 50) - 25);
    ufo.shoot_timer = UFO_SHOOT_INTERVAL; ufo.active = true;
    ufo_sound = start_ufo_sound();
}

void ufo_fire_bullet(void) {
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (!ufo_bullets[i].active) {
            int16_t dx = (int16_t)(ship.x >> 8) - (int16_t)(ufo.x >> 8);
            int16_t dy = (int16_t)(ship.y >> 8) - (int16_t)(ufo.y >> 8);
            dx += (rand() % 60) - 30; dy += (rand() % 60) - 30;
            int16_t mag = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
            if (mag > 0) { ufo_bullets[i].vx = ((int32_t)dx * BULLET_SPEED) / mag; ufo_bullets[i].vy = ((int32_t)dy * BULLET_SPEED) / mag; }
            else { ufo_bullets[i].vx = BULLET_SPEED; ufo_bullets[i].vy = 0; }
            ufo_bullets[i].x = ufo.x + ufo_bullets[i].vx; 
            ufo_bullets[i].y = ufo.y + ufo_bullets[i].vy;
            ufo_bullets[i].lifetime = BULLET_LIFETIME; ufo_bullets[i].active = true;
            pan = convert_pos_to_pan(ship.x);
            start_bullet_sound(pan);
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

    uint8_t asteroid_count = 0;
    for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            switch (asteroids[i].size)
            {
            case 0:
                asteroid_count += 8;
                break;
            case 1:
                asteroid_count += 4;
                break;
            default:
                asteroid_count++;
                break;
            }
        }
    }
    update_beat_sound(asteroid_count);

    if (!ufo.active && ufo_spawn_timer > 0) { ufo_spawn_timer--; if (ufo_spawn_timer == 0) spawn_ufo(); }
    if (ufo.active) {
        ufo.x += ufo.vx; ufo.y += ufo.vy; wrap_position(&ufo.x, &ufo.y);
        int16_t ux = (int16_t)(ufo.x >> 8);
        if ((ux < -20) || (ux > SCREEN_WIDTH + 20)) { 
            ufo.active = false; 
            ufo_spawn_timer = UFO_SPAWN_TIME; 
            stop_interpolated_sound(ufo_sound);
        }
        if (ufo.active && ship.active) { ufo.shoot_timer--; if (ufo.shoot_timer == 0) { ufo_fire_bullet(); ufo.shoot_timer = UFO_SHOOT_INTERVAL; } }
    }
    
    if (ship.active) {
        if (ship.vx > 0) ship.vx -= SHIP_FRICTION;
        else if (ship.vx < 0) ship.vx += SHIP_FRICTION;
        if (ship.vy > 0) ship.vy -= SHIP_FRICTION;
        else if (ship.vy < 0) ship.vy += SHIP_FRICTION;
        
        ship.x += ship.vx;
        ship.y += ship.vy;
        wrap_position(&ship.x, &ship.y);
    }

    for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].vx; particles[i].y += particles[i].vy;
            particles[i].vx = (particles[i].vx * 95) / 100; particles[i].vy = (particles[i].vy * 95) / 100;
            particles[i].lifetime--; if (particles[i].lifetime == 0) particles[i].active = false;
        }
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
        if (ufo_bullets[i].active) {
            ufo_bullets[i].x += ufo_bullets[i].vx;
            ufo_bullets[i].y += ufo_bullets[i].vy;
            wrap_position(&ufo_bullets[i].x, &ufo_bullets[i].y);
            
            ufo_bullets[i].lifetime--;
            if (ufo_bullets[i].lifetime == 0) {
                ufo_bullets[i].active = false;
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

        bool hit = false;  
        
        for (uint8_t j = 0; j < MAX_ASTEROIDS; j++) {
            if (!asteroids[j].active) continue;
            
            uint8_t asteroid_radius = (asteroids[j].size == 0) ? 16 : 
                                     (asteroids[j].size == 1) ? 10 : 6;
            
            if (check_collision(bullets[i].x, bullets[i].y, 1,
                               asteroids[j].x, asteroids[j].y, asteroid_radius)) {
                bullets[i].active = false;
                asteroids[j].active = false;

                uint8_t pc = (asteroids[j].size == 0) ? 8 : (asteroids[j].size == 1) ? 6 : 4;
                spawn_particles(asteroids[j].x, asteroids[j].y, pc);
                pan = convert_pos_to_pan(asteroids[i].x);
                play_explosion_sound(asteroids[j].size, pan);
                
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
                hit = true;
                break;
            }
        }
        if (!hit && ufo.active && bullets[i].active) {
            if (check_collision(bullets[i].x, bullets[i].y, 1, ufo.x, ufo.y, 12)) {
                bullets[i].active = false; ufo.active = false; ufo_spawn_timer = UFO_SPAWN_TIME;
                pan = convert_pos_to_pan(ufo.x);
                play_explosion_sound(0, pan); 
                spawn_particles(ufo.x, ufo.y, 10);
                stop_interpolated_sound(ufo_sound); 
                score += 200;
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
                
                pan = convert_pos_to_pan(ufo.x);
                play_explosion_sound(0, pan);
                spawn_ship_shatter();
                
                if (lives == 0) {
                    game_over = true;
                    play_game_over_sound();
                    if (ufo.active) {
                        ufo.active = false;
                        stop_interpolated_sound(ufo_sound);
                    }
                } else {
                    init_ship();
                }
                break;
            }
        }
        for (uint8_t i = 0; i < MAX_BULLETS; i++) {
            if (!ufo_bullets[i].active) continue;
            if (check_collision(ufo_bullets[i].x, ufo_bullets[i].y, 1, ship.x, ship.y, 8)) {
                bullets[i].active = false; ship.active = false; lives--; 
                pan = convert_pos_to_pan(ufo.x);
                play_explosion_sound(0, pan); 
                spawn_ship_shatter();
                if (lives == 0) { game_over = true; play_game_over_sound(); } else init_ship();
            }
        }
        if (ufo.active && check_collision(ship.x, ship.y, 8, ufo.x, ufo.y, 12)) {
            ship.active = false; ufo.active = false; ufo_spawn_timer = UFO_SPAWN_TIME; lives--;
            pan = convert_pos_to_pan(ufo.x);
            spawn_ship_shatter();
            play_explosion_sound(0, pan); stop_interpolated_sound(ufo_sound);
            if (lives == 0) { game_over = true; play_game_over_sound(); } else init_ship();
        }
    }

    for (uint8_t i = 0; i < MAX_SHIP_DEBRIS; i++) {
        if (ship_debris[i].active) {
            ship_debris[i].x += ship_debris[i].vx;
            ship_debris[i].y += ship_debris[i].vy;
            ship_debris[i].angle = (ship_debris[i].angle + ship_debris[i].rot_speed) & 63;
            ship_debris[i].lifetime--;
            if (ship_debris[i].lifetime == 0) ship_debris[i].active = false;
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

    for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            int16_t px = (int16_t)(particles[i].x >> 8), py = (int16_t)(particles[i].y >> 8);
            if (particles[i].lifetime > 10 || (frame_counter & 1)) {
                draw_pixel2buffer(WHITE, px, py, buffer); draw_pixel2buffer(WHITE, px+1, py, buffer);
                draw_pixel2buffer(WHITE, px, py+1, buffer); draw_pixel2buffer(WHITE, px-1, py, buffer);
                draw_pixel2buffer(WHITE, px, py-1, buffer);
            }
        }
    }

    if (ufo.active) draw_rotated_polygon(ufo_shape, 12, ufo.x, ufo.y, 32, buffer);
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            int16_t bx = (int16_t)(bullets[i].x >> 8);
            int16_t by = (int16_t)(bullets[i].y >> 8);
            draw_pixel2buffer(WHITE, bx, by, buffer);
            draw_pixel2buffer(WHITE, bx + 1, by, buffer);
            draw_pixel2buffer(WHITE, bx, by + 1, buffer);
            draw_pixel2buffer(WHITE, bx + 1, by + 1, buffer);
        }
        if (ufo_bullets[i].active) {
            int16_t bx = (int16_t)(ufo_bullets[i].x >> 8);
            int16_t by = (int16_t)(ufo_bullets[i].y >> 8);
            draw_pixel2buffer(WHITE, bx, by, buffer);
            draw_pixel2buffer(WHITE, bx + 1, by, buffer);
            draw_pixel2buffer(WHITE, bx, by + 1, buffer);
            draw_pixel2buffer(WHITE, bx + 1, by + 1, buffer);
        }
    }
    
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

    for (uint8_t i = 0; i < MAX_SHIP_DEBRIS; i++) {
        if (ship_debris[i].active) {
            uint8_t next = (i + 1 < 4) ? (i + 1) : 0;
            int16_t sx = scaled_sin[ship_debris[i].angle];
            int16_t cx = scaled_cos[ship_debris[i].angle];

            int16_t x1 = ((int16_t)ship_shape[i][0] * cx - (int16_t)ship_shape[i][1] * sx) >> 8;
            int16_t y1 = ((int16_t)ship_shape[i][0] * sx + (int16_t)ship_shape[i][1] * cx) >> 8;
            int16_t x2 = ((int16_t)ship_shape[next][0] * cx - (int16_t)ship_shape[next][1] * sx) >> 8;
            int16_t y2 = ((int16_t)ship_shape[next][0] * sx + (int16_t)ship_shape[next][1] * cx) >> 8;

            draw_line2buffer(WHITE, 
                (int16_t)(ship_debris[i].x >> 8) + x1, (int16_t)(ship_debris[i].y >> 8) + y1,
                (int16_t)(ship_debris[i].x >> 8) + x2, (int16_t)(ship_debris[i].y >> 8) + y2, 
                buffer);
        }
    }
    
    set_cursor(10, 10);
    sprintf(text_buffer, "%u", score);
    draw_string2buffer(text_buffer, buffer);
    
    set_cursor(10, 25);
    sprintf(text_buffer, "LVS: %u", lives);
    draw_string2buffer(text_buffer, buffer);

    set_cursor(10, 40);
    sprintf(text_buffer, "LVL: %u", level);
    draw_string2buffer(text_buffer, buffer);
    
    if (game_over) {
        set_cursor(260, 170);
        draw_string2buffer("GAME OVER", buffer);
        set_cursor(255, 190);
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

    precalculate_rotation_tables(); 
    init_sound();
    
    erase_buffer(buffers[0]);
    erase_buffer(buffers[1]);
    
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
    
    active_buffer = 0;
    switch_buffer(buffers[active_buffer]);

    set_cursor(267, 140);
    draw_string2buffer("*** ASTEROIDS ***", buffers[active_buffer]);
    set_cursor(260, 180);
    draw_string2buffer("Arrows:Rotate/Thrust", buffers[active_buffer]);
    set_cursor(265, 195);
    draw_string2buffer("Space:Fire ESC:Quit", buffers[active_buffer]);
    set_cursor(287, 220);
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


    uint8_t v; 
    v = RIA.vsync;
    
    while (running) {
        if (v == RIA.vsync) {
            continue; 
        } else {
            v = RIA.vsync; 
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
                
                pan = convert_pos_to_pan(ship.x);
                start_thrust_sound(pan);
            } else {
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
        
        update_sound();
        
        erase_buffer(buffers[!active_buffer]);
        draw_game(buffers[!active_buffer]);
        
        switch_buffer(buffers[!active_buffer]);
        active_buffer = !active_buffer;
    }
    
    return 0;
}