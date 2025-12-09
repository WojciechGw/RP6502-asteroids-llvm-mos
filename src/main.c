#include <rp6502.h>
#include <time.h>
#include "colors.h"
#include "bitmap_graphics_db.h"
#include "usb_hid_keys.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "ship.h"
#include "bullet.h"
#include "asteroid.h"
#include "sin_cos_tables.h"
#include "definitions.h"


// XRAM locations
#define KEYBOARD_INPUT 0xFF10 // KEYBOARD_BYTES of bitmask data

// 256 bytes HID code max, stored in 32 uint8
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};

// keystates[code>>3] gets contents from correct byte in array
// 1 << (code&7) moves a 1 into proper position to mask with byte contents
// final & gives 1 if key is pressed, 0 if not
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))


uint16_t buffers[2];
uint8_t active_buffer = 0;

// Function prototypes
void init_game(void);
void update_game(void);



// Initialize game
void init_game(void) {
    init_ship();
    init_bullets();
    init_asteroids();
}

// Update game state (move ship, handle wrapping around screen)
void update_game(void) {
    // Update ship position based on velocity (scaled down for slower movement)
    
    player_ship.x += (player_ship.vel_x / 2) ;  // Slow down movement by dividing by 2
    player_ship.y += player_ship.vel_y / 2;

    // Screen wrapping
    if (player_ship.x < 0) player_ship.x += SCREEN_WIDTH;
    if (player_ship.x >= SCREEN_WIDTH) player_ship.x -= SCREEN_WIDTH;
    if (player_ship.y < 0) player_ship.y += SCREEN_HEIGHT;
    if (player_ship.y >= SCREEN_HEIGHT) player_ship.y -= SCREEN_HEIGHT;

    // Check for collisions with asteroids
    // for (int i = 0; i < MAX_ASTEROIDS; ++i) {
    //     if (check_collision(player_ship, asteroids[i])) {
    //         // Collision detected
    //         player_ship.lives--;  // Subtract one life
    //         asteroids[i].active = false;
    //         // printf("lives: %i\n", player_ship.lives);

    //         if (player_ship.lives > 0) {
    //             relocate_ship();  // Move ship to safe location
    //         } else {
    //             // Restart game if lives reach 0
    //             init_game();  // Reset the game state
    //         }
    //         break;  // Only handle one collision at a time
    //     }
    // }

   

    // Update bullets
    update_bullets();

    // // check bullet asteroid collisions
    checkBulletAsteroidCollision(bullets, MAX_BULLETS, asteroids, MAX_ASTEROIDS);

    // // Update asteroids
    update_asteroids();

     if (active_asteroids < (MAX_ASTEROIDS - 3)) {
        for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
            if (!asteroids[i].active) {
                create_asteroid(i);
                break;
            }
        }
    }
}

void WaitForAnyKey(){

    xregn(0, 0, 0, 1, KEYBOARD_INPUT);
    RIA.addr0 = KEYBOARD_INPUT;
    RIA.step0 = 0;
    while (RIA.rw0 & 1)
        ;
}



int main() {
    bool handled_key = false;
    bool paused = true;


    // WaitForAnyKey();

    init_bitmap_graphics(0xFF00, 0x0000, 1, 3, SCREEN_WIDTH, SCREEN_HEIGHT, 1);

    // assign address for each buffer
    buffers[0] = 0x0000;
    buffers[1] = BUFFER_SIZE;
    erase_buffer(buffers[active_buffer]);
    erase_buffer(buffers[!active_buffer]);

    active_buffer = 0;
    switch_buffer(buffers[active_buffer]);

    // Initialize the game
    init_game();

    long previous_time = clock();
    long lag = 0;
    long shot_time = 0;

    while (1) {

        long current_time = clock();
        long elapsed = current_time - previous_time;
        previous_time = current_time;
        lag += elapsed;

        xregn( 0, 0, 0, 1, KEYBOARD_INPUT);
        RIA.addr0 = KEYBOARD_INPUT;
        RIA.step0 = 0;
        

        // fill the keystates bitmask array
        for (int i = 0; i < KEYBOARD_BYTES; i++) {
            uint8_t new_keys;
            RIA.addr0 = KEYBOARD_INPUT + i;
            new_keys = RIA.rw0;
            keystates[i] = new_keys;
        }
        // printf("keystates[0]: %i, %i\n", keystates[0], keystates[0] & 1);
        
        if (!(keystates[0] & 1)) {
            // if (!handled_key) { // handle only once per single keypress
                if (key(KEY_LEFT))
                    player_ship.angle = (player_ship.angle - 1 + NUM_ANGLES) % NUM_ANGLES;  // Rotate left (finer step)
                if (key(KEY_RIGHT))
                    player_ship.angle = (player_ship.angle + 1) % NUM_ANGLES;  // Rotate right (finer step)
                if (key(KEY_UP)) 
                    apply_thrust();
				if (key(KEY_SPACE))
                    if (current_time > (shot_time + SHOT_DELAY)) {
                        fire_bullet();  // Fire bullet on space press
                        shot_time = current_time;
                    }
				
                if (key(KEY_ESC)) {
                    break;
                }
                // handled_key = true;
            // }
        } else if (show_fire && (keystates[0] & 1))
        {
            show_fire = false;
        }


        // Update the game state at a fixed interval
        while (lag >= CLOCKS_PER_UPDATE) {
            update_game();
            lag -= CLOCKS_PER_UPDATE;
        }

        erase_buffer(buffers[!active_buffer]);

        draw_ship(buffers[!active_buffer]);  // Draw ship with white lines
        draw_bullets(buffers[!active_buffer]);
        draw_asteroids(buffers[!active_buffer]);  // Add this line to draw asteroids

        switch_buffer(buffers[!active_buffer]);

        // change active buffer
        active_buffer = !active_buffer;
    };


    return 0;
}