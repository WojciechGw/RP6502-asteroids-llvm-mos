#ifndef ASTEROID_H
#define ASTEROID_H

#include <stdint.h>
#include <stdbool.h>
#include "bullet.h"
#include "ship.h"
#include "definitions.h"
#include "colors.h"

typedef struct {
    int16_t x, y;            // Position
    int16_t vel_x, vel_y;     // Velocity
    uint8_t scale;            // Scale of the asteroid
    uint8_t shape;            // index of the shape
    bool active;     // Whether the asteroid is active (1 = active, 0 = inactive)
} Asteroid;

extern Asteroid asteroids[MAX_ASTEROIDS];
extern uint8_t active_asteroids;

void create_asteroid(uint8_t);
void init_asteroids(void);
void update_asteroids(void);
void draw_asteroids(uint16_t buffer);
bool check_collision(Ship ship, Asteroid asteroid);
int squaredDistance(int x1, int y1, int x2, int y2);
void checkBulletAsteroidCollision(Bullet bullets[], int numBullets, Asteroid asteroids[], int numAsteroids);
void handleAsteroidHit(Asteroid* asteroid, Asteroid asteroids[], int numAsteroids);
void splitAsteroid(Asteroid* asteroid, Asteroid asteroids[], int numAsteroids);

#endif // ASTEROID_H
