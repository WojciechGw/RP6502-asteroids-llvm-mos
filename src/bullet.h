#ifndef BULLET_H
#define BULLET_H

#include <stdint.h>
#include <stdbool.h>
#include "definitions.h"
#include "colors.h"


typedef struct {
    int16_t x, y;            // Position
    int16_t vel_x, vel_y;     // Velocity
    bool active;              // Whether the bullet is active or not
} Bullet;

extern Bullet bullets[MAX_BULLETS];

void init_bullets(void);
void fire_bullet(void);
void update_bullets(void);
void draw_bullets(uint16_t buffer);

#endif // BULLET_H
