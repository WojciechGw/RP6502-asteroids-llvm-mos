// sound.h
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

// --- Helper Constants for ezpsg ---
// These make the code in sound.c more readable.


// Panning
#define EZPSG_PAN_LEFT    0x80
#define EZPSG_PAN_RIGHT   0x40
#define EZPSG_PAN_CENTER  0xC0 // Both left and right

// --- Your Game's Sound Functions ---

void init_sound(void);
void update_sound(void);

void play_shoot_sound(void);
void play_explosion_sound(uint8_t size);
void play_game_over_sound(void);

// Split into two functions for continuous sounds
void start_thrust_sound(void);
void stop_thrust_sound(void);

#endif // SOUND_H
