// sound.c
#include "sound.h"
#include "ezpsg.h"
#include <rp6502.h>
#include <stdbool.h>

// Keep track of the continuous thrust sound
static bool is_thrust_playing = false;
static uint16_t thrust_channel_xaddr = 0xFFFF;

// Initialize the sound system
void init_sound(void) {

    ezpsg_init(0xFEC0);
}

// This function must be called every frame in your main game loop
void update_sound(void) {

    ezpsg_tick(1);

    // If the thrust sound channel has finished on its own, reset our flag.
    if (is_thrust_playing && thrust_channel_xaddr != 0xFFFF) {
        uint8_t pan_gate;
        RIA.addr0 = thrust_channel_xaddr + 6; // Offset 6 is pan_gate in the struct
        RIA.step0 = 0;
        pan_gate = RIA.rw0;
        if ((pan_gate & 0x01) == 0) { // If gate bit is 0, sound is off
            is_thrust_playing = false;
        }
    }
}

// --- Sound Effect Definitions ---

void play_shoot_sound(void) {
    ezpsg_play_note(c5,   // note
                        3,    // duration
                        3,    // release
                        62,   // duty
                        0x00, // vol_attack
                        0x00, // vol_decay
                        0x04,
                        0);   // pan
    
}

void play_explosion_sound(uint8_t size) {
    ezpsg_play_note(c1 + (size * 3),  // note
                        4,    // duration
                        0,    // release
                        64,   // duty
                        0x01, // vol_attack
                        0xF8, // vol_decay
                        EZPSG_WAVE_NOISE,
                        0);   // pan
 
}

void play_game_over_sound(void) {
    ezpsg_play_note(
        g3,               // note: A medium-low G
        3,               // duration: Long hold
        3,               // release: Long fade
        0xFF,             // duty (square wave)
        0xD0,             // vol_attack: Soft attack
        0x90,             // vol_decay: Slow decay
        EZPSG_WAVE_SQUARE,
        EZPSG_PAN_CENTER
    );
}

// This is now split into two functions to handle the continuous nature of the sound
void start_thrust_sound(void) {
    if (!is_thrust_playing) {
        thrust_channel_xaddr = ezpsg_play_note(
            c2,   // note: low C                                                                              │
            10,   // duration: long, sustained                                                                │
            10,   // release: short fade when done                                                            │
            28,    // duty: not used for sine                                                                  │
            0x00, // vol_attack: soft attack, medium volume                                                   │
            0x00, // vol_decay: sustain                                                                       │
            EZPSG_WAVE_NOISE, 
            0);   // pan: center      
  
        if (thrust_channel_xaddr != 0xFFFF) {
            is_thrust_playing = true;
        }
    }
}

void stop_thrust_sound(void) {
    is_thrust_playing = false;
}
