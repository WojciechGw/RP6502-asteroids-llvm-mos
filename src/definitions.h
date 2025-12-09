#ifndef DEFINITIONS_H
#define DEFINITIONS_H


#define SCREEN_WIDTH  600         // Screen width
#define SCREEN_HEIGHT  360        // Screen height
#define BUFFER_SIZE  0x7080

#define CLOCKS_PER_SEC 100
#define TARGET_FPS 60
#define CLOCKS_PER_UPDATE (CLOCKS_PER_SEC / TARGET_FPS)
#define SHOT_DELAY 30

#define BIT_SHIFT 5 // for more precise calculatins


#define MAX_BULLETS 3 // Maximum number of bullets
#define BULLET_SPEED 2  // Bullet speed (can be adjusted)
#define BULLET_SIZE 2  // Example bullet size

#define MAX_SHIP_SPEED 10  // Maximum ship speed (adjustable)
#define MAX_SHIP_SPEED_SQ (MAX_SHIP_SPEED * MAX_SHIP_SPEED)  // Square of the max speed
#define THRUST_POWER 2
#define SHIP_LENGTH  8           // Example ship length (adjust to match your ship)
#define SAFE_ZONE_RADIUS  5*SHIP_LENGTH // The radius of the "safe zone" around the center


#define NUM_ASTEROIDS_SHAPES 3
#define MAX_ASTEROIDS 6
#define INITIAL_ASTEROIDS 3
#define INITIAL_ASTEROID_SPEED 0.2
#define MIN_ASTEROID_SIZE 1  // Minimum size of the asteroid
#define INITIAL_ASTEROID_SIZE 6  // Initial size of the large asteroid


#endif 