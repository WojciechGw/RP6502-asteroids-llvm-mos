#include "ship.h"
#include "sin_cos_tables.h"
#include "asteroid.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "colors.h"
#include "bitmap_graphics_db.h"


Ship player_ship;
extern Asteroid asteroids[MAX_ASTEROIDS];
bool show_fire = false;


// Function to initialize the ship
void init_ship(void) {
    player_ship.x = SCREEN_WIDTH / 2;  // Start at the center of an 800x600 window
    player_ship.y = SCREEN_HEIGHT / 2;
    player_ship.vel_x = 0;
    player_ship.vel_y = 0;
    player_ship.angle = 0;  // Start facing up (index 0 of the finer angles)
    player_ship.size = SHIP_LENGTH;  // Define the size of the ship
    player_ship.lives = 4;
}

void relocate_ship() {
    bool safe_position_found = false;
    uint8_t attempts = 100;  // Limit attempts to find a safe position

    while (!safe_position_found && attempts > 0) {
        // Reposition ship randomly near the center of the screen
        player_ship.x = SCREEN_WIDTH / 2;
        player_ship.y = SCREEN_HEIGHT / 2;
        player_ship.vel_x = 0;
        player_ship.vel_y = 0;
        // printf("attempts: %i, x: %i, y: %i\n", attempts, player_ship.x, player_ship.y);

        // Check the squared distance from all asteroids
        safe_position_found = true;
        for (int i = 0; i < MAX_ASTEROIDS; ++i) {
            int dx = player_ship.x - asteroids[i].x;
            int dy = player_ship.y - asteroids[i].y;
            int distance_squared = dx * dx + dy * dy;

            // 5 ship lengths squared
            int minimum_safe_distance_squared = 5 * player_ship.size * 5 * player_ship.size;

            // If too close (less than 5 ship lengths), retry
            if (distance_squared < minimum_safe_distance_squared) {
                safe_position_found = false;
                break;
            }
        }
        --attempts;
    }
}

// Function to apply thrust (only apply if within speed limit)
void apply_thrust(void) {
   
    int16_t new_vel_x = player_ship.vel_x + (sin_fix8[player_ship.angle] * THRUST_POWER) / 256;
    int16_t new_vel_y = player_ship.vel_y - (cos_fix8[player_ship.angle] * THRUST_POWER) / 256;

    // Calculate the squared magnitude of the new velocity
    uint16_t new_velocity_magnitude_sq = new_vel_x * new_vel_x + new_vel_y * new_vel_y;

    // Only apply thrust if the new velocity magnitude is within the limit
    if (new_velocity_magnitude_sq <= MAX_SHIP_SPEED_SQ) {
        player_ship.vel_x = new_vel_x;
        player_ship.vel_y = new_vel_y;
        show_fire = true;  // Show fire effect if thrust is applied
    }
}

// Function to draw the ship
void draw_ship(uint16_t buffer) {
    int16_t x0 = player_ship.x;
    int16_t y0 = player_ship.y;
    uint8_t angle_index = player_ship.angle % NUM_ANGLES;
    uint8_t size = player_ship.size;

    // Ship dimensions: nose length, half width, rear length
    uint8_t nose_length = size;
    uint8_t width_half = size / 8;
    uint8_t rear_length = size / 2;
    uint8_t wing_extension = size / 6;

    // Relative positions of the ship's points from the center:
    // Nose (front of the ship)
    uint16_t nose_x = (sin_fix8[angle_index] * nose_length) / 256;
    uint16_t nose_y = -(cos_fix8[angle_index] * nose_length) / 256;

    // Rear-left point of the ship
    uint16_t rear_left_x = (sin_fix8[(angle_index + 16) % NUM_ANGLES] * (width_half + rear_length)) / 256;
    uint16_t rear_left_y = -(cos_fix8[(angle_index + 16) % NUM_ANGLES] * (width_half + rear_length)) / 256;

    // Rear-right point of the ship
    uint16_t rear_right_x = (sin_fix8[(angle_index + 32) % NUM_ANGLES] * (width_half + rear_length)) / 256;
    uint16_t rear_right_y = -(cos_fix8[(angle_index + 32) % NUM_ANGLES] * (width_half + rear_length)) / 256;

    // Left wing tip
    uint16_t wing_left_x = (sin_fix8[(angle_index + 16) % NUM_ANGLES] * (width_half + wing_extension)) / 256;
    uint16_t wing_left_y = -(cos_fix8[(angle_index + 16) % NUM_ANGLES] * (width_half + wing_extension)) / 256;

    // Right wing tip
    uint16_t wing_right_x = (sin_fix8[(angle_index + 32) % NUM_ANGLES] * (width_half + wing_extension)) / 256;
    uint16_t wing_right_y = -(cos_fix8[(angle_index + 32) % NUM_ANGLES] * (width_half + wing_extension)) / 256;

    uint16_t line_color = WHITE; 

    draw_line2buffer(line_color, x0 + nose_x, y0 + nose_y, x0 + rear_left_x, y0 + rear_left_y, buffer);
    draw_line2buffer(line_color, x0 + rear_left_x, y0 + rear_left_y, x0 + wing_left_x, y0 + wing_left_y, buffer);
    draw_line2buffer(line_color, x0 + wing_left_x, y0 + wing_left_y, x0 + wing_right_x, y0 + wing_right_y, buffer);
    draw_line2buffer(line_color, x0 + wing_right_x, y0 + wing_right_y, x0 + rear_right_x, y0 + rear_right_y, buffer);
    draw_line2buffer(line_color, x0 + rear_right_x, y0 + rear_right_y, x0 + nose_x, y0 + nose_y, buffer);

    if (show_fire) {
        // Add a triangle for fire attached to the rear
        uint8_t fire_length = size / 2;

        // Calculate the fire's start point, which is at the rear of the ship
        uint16_t fire_start_x = rear_left_x + (rear_right_x - rear_left_x) / 2;
        uint16_t fire_start_y = rear_left_y + (rear_right_y - rear_left_y) / 2;

        // Calculate the fire's end point, extending from the rear
        uint16_t fire_end_x = -(sin_fix8[angle_index] * fire_length) / 256;
        uint16_t fire_end_y = (cos_fix8[angle_index] * fire_length) / 256;
        
        // Draw the fire triangle
        draw_line2buffer(line_color, x0 + wing_left_x, y0 + wing_left_y, x0 + fire_end_x, y0 + fire_end_y, buffer);
        draw_line2buffer(line_color, x0 + wing_right_x, y0 + wing_right_y, x0 + fire_end_x, y0 + fire_end_y, buffer);
        // MoveToEx(hdc, x0 + wing_left_x, y0 + wing_left_y, NULL);
        // LineTo(hdc, x0 + fire_end_x, y0 + fire_end_y);
        // LineTo(hdc, x0 + wing_right_x, y0 + wing_right_y);
    }
    
}
