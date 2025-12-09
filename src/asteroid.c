#include <rp6502.h>
#include "asteroid.h"
#include "bullet.h"
#include "ship.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "colors.h"
#include "bitmap_graphics_db.h"


Asteroid asteroids[MAX_ASTEROIDS];
uint8_t active_asteroids;

// Base asteroid shape (relative coordinates) using int array
static const int base_asteroid_shape[][13][2] = {
    {{0, 4}, {1, 2}, {4, 0}, {6, 2}, {8, 2}, {9, 5}, {7, 6}, {7, 8}, {4, 9}, {3, 8}, {2, 7}, {1, 7}, {0, 4}},
    {{0, 6}, {0, 3}, {2, 2}, {3, 0}, {5, 0}, {6, 2}, {8, 2}, {9, 5}, {7, 6}, {7, 8}, {4, 9}, {3, 7}, {0, 6}},
    {{0, 3}, {3, 4}, {1, 1}, {3, 0}, {7, 0}, {9, 2}, {9, 5}, {7, 7}, {6, 8}, {1, 8}, {2, 5}, {0, 6}, {0, 3}},
    };

static const int num_asteroid_points = 13;

// Function to calculate squared distance (avoiding sqrt and float)
int squaredDistance(int x1, int y1, int x2, int y2) {
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}


void create_asteroid(uint8_t i) {
    // Randomly select an edge: 0 = top, 1 = bottom, 2 = left, 3 = right
    uint8_t edge = random(0, 4);
    
    // Set initial position based on the selected edge
    switch (edge) {
        case 0:  // Top edge
            asteroids[i].x = random(0, SCREEN_WIDTH << BIT_SHIFT);
            asteroids[i].y = 0;
            break;
        case 1:  // Bottom edge
            asteroids[i].x = random(0, SCREEN_WIDTH << BIT_SHIFT);
            asteroids[i].y = (SCREEN_HEIGHT - 1) << BIT_SHIFT;

            break;
        case 2:  // Left edge
            asteroids[i].x = 0;
            asteroids[i].y = random(0, SCREEN_HEIGHT << BIT_SHIFT);
            break;
        case 3:  // Right edge
            asteroids[i].x = (SCREEN_WIDTH - 1) << BIT_SHIFT;
            asteroids[i].y = random(0, SCREEN_HEIGHT << BIT_SHIFT);
            break;
    }

    asteroids[i].shape = random(0, NUM_ASTEROIDS_SHAPES);
    printf("shape: %i %i edge: %i %i %i ", i, asteroids[i].shape, edge, asteroids[i].x >> BIT_SHIFT, asteroids[i].y >> BIT_SHIFT);

    // Set asteroid size (scale)
    asteroids[i].scale = INITIAL_ASTEROID_SIZE / 2;

    asteroids[i].vel_x = (random(0, INITIAL_ASTEROID_SPEED * 1000) - (INITIAL_ASTEROID_SPEED * 500)) / 6;  // Random velocity between -1 and 1
    asteroids[i].vel_y = (random(0, INITIAL_ASTEROID_SPEED * 1000) - (INITIAL_ASTEROID_SPEED * 500)) / 6;

    
    if (asteroids[i].vel_x == 0) asteroids[i].vel_x = 512;
    if (asteroids[i].vel_y == 0) asteroids[i].vel_y = 512;

    // // Adjust velocities to ensure the asteroid moves inward
    if (edge == 0) asteroids[i].vel_y *= -1; // Moving down
    if (edge == 1) asteroids[i].vel_y *= -1; // Moving up
    if (edge == 2) asteroids[i].vel_x *= -1; // Moving right
    if (edge == 3) asteroids[i].vel_x *= -1; // Moving left

    printf(", vel_x: %i, vel_y: %i\n", asteroids[i].vel_x, asteroids[i].vel_y);


    // Activate the asteroid
    asteroids[i].active = true;
    active_asteroids++;
}


// Initialize asteroids
void init_asteroids(void) {

    for (uint8_t i = 0; i < INITIAL_ASTEROIDS; ++i) {
        create_asteroid(i);    
    }
}

// Update asteroid positions
void update_asteroids(void) {
    for (uint8_t i = 0; i < MAX_ASTEROIDS; ++i) {
        asteroids[i].x += asteroids[i].vel_x;
        asteroids[i].y += asteroids[i].vel_y;

        // Screen wrapping
        if (asteroids[i].x < 0) asteroids[i].x += SCREEN_WIDTH << BIT_SHIFT;
        if (asteroids[i].x >= SCREEN_WIDTH << BIT_SHIFT) asteroids[i].x -= SCREEN_WIDTH << BIT_SHIFT;
        if (asteroids[i].y < 0) asteroids[i].y += SCREEN_HEIGHT << BIT_SHIFT;
        if (asteroids[i].y >= SCREEN_HEIGHT << BIT_SHIFT) asteroids[i].y -= SCREEN_HEIGHT << BIT_SHIFT;

        // printf("asteroids[i].x %i, asterteroids[i].y: %i\n", asteroids[i].x, asteroids[i].y);
    }
}

// Draw the asteroids
void draw_asteroids(uint16_t buffer) {
    for (uint8_t i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].active) {
            // Scale and draw each asteroid
            int16_t x0 = asteroids[i].x >> BIT_SHIFT;
            int16_t y0 = asteroids[i].y >> BIT_SHIFT;
            uint8_t scale = asteroids[i].scale;
            uint8_t shape = asteroids[i].shape;
            // printf("scale: %i %i\n", asteroids[i].scale, scale);

            // draw_rect2buffer(WHITE, x0, y0, asteroids[i].scale, asteroids[i].scale, buffer);
            // draw_hline2buffer(WHITE, x0-scale, y0-scale, scale*2, buffer);
            // draw_hline2buffer(WHITE, x0, y0, scale, buffer);
            // draw_hline2buffer(WHITE, x0-scale, y0+scale, scale, buffer);
            // draw_vline2buffer(WHITE, x0-scale, y0-scale, scale*2, buffer);
            // draw_vline2buffer(WHITE, x0, y0, scale, buffer);
            // draw_vline2buffer(WHITE, x0+scale, y0-scale, scale, buffer);
            

            // printf("i: %i, x0: %i, y0: %i \n", i, x0, y0);
            for (uint8_t j = 1; j < num_asteroid_points; ++j) {
                // Calculate the start and end points for each line segment
                int16_t x1 = x0 + base_asteroid_shape[shape][j - 1][0] * scale % SCREEN_WIDTH;
                int16_t y1 = y0 + base_asteroid_shape[shape][j - 1][1] * scale % SCREEN_HEIGHT;
                int16_t x2 = x0 + base_asteroid_shape[shape][j][0] * scale % SCREEN_WIDTH;
                int16_t y2 = y0 + base_asteroid_shape[shape][j][1] * scale % SCREEN_HEIGHT;
                if (x1 < 0) x1 = 0;
                if (x2 < 0) x2 = 0;
                if (y1 < 0) y1 = 0;
                if (y2 < 0) y2 = 0;
                // printf("x1: %i, y1: %i, x2: %i, y2: %i\n", x1, y1, x2, y2);

                // Call the custom draw_line function to draw each segment
                draw_line2buffer(WHITE, x1, y1, x2, y2, buffer);
                // draw_pixel2buffer(WHITE, x1, y1, buffer);
            }
            
        }
    }
}

bool check_collision(Ship ship, Asteroid asteroid) {
    // Calculate the squared distance between the ship and the asteroid
    // int16_t dx = ship.x - asteroid.x;
    // int16_t dy = ship.y - asteroid.y;
    // int distance_squared = dx * dx + dy * dy;
    if (asteroid.active) {
        uint16_t distance_squared = squaredDistance(ship.x, ship.y, asteroid.x, asteroid.y);

        // Squared sum of the radii
        uint16_t ship_radius = ship.size * 2;
        uint16_t asteroid_radius = asteroid.scale;  // Adjust asteroid size based on scale
        uint16_t combined_radius_squared = (ship_radius + asteroid_radius) * (ship_radius + asteroid_radius);
        // printf("asteroid_radius: %i, combined_radius_squared: %i\n", asteroid_radius, combined_radius_squared);

        // Check if the squared distance is less than the squared sum of the radii
        return distance_squared < combined_radius_squared;
    } 
    return false;

}

void checkBulletAsteroidCollision(Bullet bullets[], int numBullets, Asteroid asteroids[], int numAsteroids) {
    for (uint8_t i = 0; i < numBullets; i++) {
        if (!bullets[i].active) continue;  // Skip inactive bullets
        for (uint8_t j = 0; j < numAsteroids; j++) {
            if (!asteroids[j].active) continue;  // Skip inactive asteroids
            // Calculate squared distance between bullet and asteroid center
            uint16_t distSq = squaredDistance(bullets[i].x, bullets[i].y, asteroids[j].x, asteroids[j].y);
             // Squared sum of the radii
            uint8_t bullet_radius = BULLET_SIZE;
            uint16_t asteroid_radius = asteroids[i].scale;  // Adjust asteroid size based on scale
            uint16_t combined_radius_squared = (bullet_radius + asteroid_radius) * (bullet_radius + asteroid_radius);
            if (distSq <= combined_radius_squared) {
                // Bullet hit the asteroid
                bullets[i].active = 0;   // Deactivate bullet
                handleAsteroidHit(&asteroids[j], asteroids, numAsteroids);  // Handle asteroid splitting or destruction
                break;  // Move to the next bullet after handling this collision
            }
        }
    }
}

void handleAsteroidHit(Asteroid* asteroid, Asteroid asteroids[], int numAsteroids) {
    if (asteroid->scale > MIN_ASTEROID_SIZE) {
        // Split the asteroid into two smaller asteroids
        splitAsteroid(asteroid, asteroids, numAsteroids);
    } else {
        // Asteroid is at minimum size, destroy it
        asteroid->active = 0;  // Deactivate asteroid
        active_asteroids--;
        
    }
}

void splitAsteroid(Asteroid* asteroid, Asteroid asteroids[], int numAsteroids) {
    int newSize = asteroid->scale - 1;

    // Find two inactive asteroids slots for the new asteroids
    int firstNewAsteroidIndex = -1, secondNewAsteroidIndex = -1;
    for (uint8_t i = 0; i < numAsteroids; i++) {
        if (!asteroids[i].active) {
            if (firstNewAsteroidIndex == -1) {
                firstNewAsteroidIndex = i;
            } else {
                secondNewAsteroidIndex = i;
                break;
            }
        }
    }

    if (firstNewAsteroidIndex != -1 && secondNewAsteroidIndex != -1) {
        // Initialize the new smaller asteroids
        asteroids[firstNewAsteroidIndex].x = asteroid->x + newSize;  // Offset for split
        asteroids[firstNewAsteroidIndex].y = asteroid->y;
        asteroids[firstNewAsteroidIndex].scale = newSize;
        asteroids[firstNewAsteroidIndex].vel_x = asteroid->vel_x + 0.1;  // Adjust speed slightly
        asteroids[firstNewAsteroidIndex].vel_y = asteroid->vel_y + 0.1;
        asteroids[firstNewAsteroidIndex].active = 1;
        asteroids[firstNewAsteroidIndex].shape = random(0, NUM_ASTEROIDS_SHAPES);

        asteroids[secondNewAsteroidIndex].x = asteroid->x - newSize;  // Offset for split
        asteroids[secondNewAsteroidIndex].y = asteroid->y;
        asteroids[secondNewAsteroidIndex].scale = newSize;
        asteroids[secondNewAsteroidIndex].vel_x = asteroid->vel_x;
        asteroids[secondNewAsteroidIndex].vel_y = asteroid->vel_y - 0.1;
        asteroids[secondNewAsteroidIndex].active = 1;
        asteroids[secondNewAsteroidIndex].shape = random(0, NUM_ASTEROIDS_SHAPES);

        active_asteroids++;

        // Deactivate the original asteroid
        asteroid->active = 0;
    }
}

