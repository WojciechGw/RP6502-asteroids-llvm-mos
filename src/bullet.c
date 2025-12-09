#include "bullet.h"
#include "ship.h"
#include "sin_cos_tables.h"
#include "colors.h"
#include "bitmap_graphics_db.h"

Bullet bullets[MAX_BULLETS];

// Initialize bullets
void init_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullets[i].active = false;
    }
}

void fire_bullet(void) {
    // Find an inactive bullet
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) {
            // Activate the bullet
            bullets[i].active = true;

            // Set the bullet position to the tip of the ship (based on the ship's current rotation axis)
            bullets[i].x = player_ship.x + (sin_fix8[player_ship.angle] * player_ship.size) / 256;
            bullets[i].y = player_ship.y - (cos_fix8[player_ship.angle] * player_ship.size) / 256;

            // Set the bullet velocity based on the ship's current angle (axis) and the ship's current velocity
            bullets[i].vel_x = + (sin_fix8[player_ship.angle] * BULLET_SPEED) / 256;
            bullets[i].vel_y = - (cos_fix8[player_ship.angle] * BULLET_SPEED) / 256;

            break;  // Only fire one bullet at a time
        }
    }
}

// Update bullets
void update_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].active) {
            // Move the bullet
            bullets[i].x += bullets[i].vel_x;
            bullets[i].y += bullets[i].vel_y;

            // Deactivate if the bullet leaves the screen
            if (bullets[i].x < 0 || bullets[i].x >= SCREEN_WIDTH || bullets[i].y < 0 || bullets[i].y >= SCREEN_HEIGHT) {
                bullets[i].active = false;
            }
        }
    }
}

// Draw the bullets
void draw_bullets(uint16_t buffer) {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].active) {
            // Draw the bullet as a point (small line or dot)
            draw_pixel2buffer(WHITE, bullets[i].x, bullets[i].y, buffer);
            // MoveToEx(hdc, bullets[i].x, bullets[i].y, NULL);
            // LineTo(hdc, bullets[i].x + 1, bullets[i].y);  // Simple way to draw a point as a line
        }
    }
}