// sound.c
#include "sound.h"
#include "ezpsg.h"
#include <rp6502.h>
#include <stdbool.h>

// Keep track of the continuous thrust sound
static bool is_thrust_playing = false;
static uint16_t thrust_channel_xaddr = 0xFFFF;

// Sound effect state
typedef struct {
    uint16_t psg_addr;      // Address in PSG memory (0xFFFF = not playing)
    uint8_t frame_counter;  // For multi-frame effects
    bool active;
} SoundEffect;

// Sound channels
static SoundEffect thrust_sound = {0xFFFF, 0, false};
static SoundEffect ufo_sound = {0xFFFF, 0, false};
static uint8_t beat_timer = 0;
static uint8_t beat_interval = 60;  // Frames between beats (starts slow)
static bool beat_low = true;         // Alternates between two tones

// Initialize the sound system
void init_sound(void) {

    ezpsg_init(0xFEC0);
}


// --- Sound Effect Definitions ---

void play_shoot_sound(void) {
    ezpsg_play_note(f5,   // note
                        5,    // duration
                        0,    // release
                        178,   // duty
                        0x00, // vol_attack
                        0xFA, // vol_decay
                        0x38,
                        0);   // pan
    
}

void play_explosion_sound(uint8_t size) {
    ezpsg_play_note(a0 + (size * 2),  // note
                        3,    // duration
                        0,    // release
                        135,   // duty
                        0x00, // vol_attack
                        0x00, // vol_decay
                        0x48,
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
        0
    );
}

void start_thrust_sound(void) {
    if (!is_thrust_playing) {
        thrust_channel_xaddr = ezpsg_play_note(
            c2,  
            10,  
            10,  
            28, 
            0x00,
            0x00,
            EZPSG_WAVE_NOISE, 
            0); 
  
        if (thrust_channel_xaddr != 0xFFFF) {
            is_thrust_playing = true;
        }
    }
}

void stop_thrust_sound(void) {
    is_thrust_playing = false;
}

// Start UFO sound (continuous warbling)
void play_ufo_sound(void) {
    if (!ufo_sound.active) {
        ufo_sound.psg_addr = ezpsg_play_note(
            c5,           // Mid-range warble
            255,          // Long duration
            10,           // Quick release
            0xFF,         // Square wave
            0x60,         // Medium volume
            0x50,         // Sustain
            EZPSG_WAVE_SQUARE,
            EZPSG_PAN_CENTER
        );
        ufo_sound.active = true;
        ufo_sound.frame_counter = 0;
    }
}

// Stop UFO sound
void stop_ufo_sound(void) {
    if (ufo_sound.active) {
        if (ufo_sound.psg_addr != 0xFFFF) {
            uint8_t pan_gate;
            RIA.addr0 = thrust_channel_xaddr + 6; // Offset 6 is pan_gate in the struct
            RIA.step0 = 0;
            pan_gate = RIA.rw0;
            if ((pan_gate & 0x01) == 0) { // If gate bit is 0, sound is off
                is_thrust_playing = false;
            }
        }
        ufo_sound.psg_addr = 0xFFFF;
        ufo_sound.active = false;
    }
}

// Update UFO warble effect
void update_ufo_warble(void) {
    if (ufo_sound.active && ufo_sound.psg_addr != 0xFFFF) {
        ufo_sound.frame_counter++;
        
        // Alternate between two pitches every 15 frames
        if ((ufo_sound.frame_counter % 15) == 0) {
            stop_ufo_sound();
            
            // Alternate between c5 and d5
            uint16_t note = ((ufo_sound.frame_counter / 15) & 1) ? d5 : c5;
            
            ufo_sound.psg_addr = ezpsg_play_note(
                note,
                255,
                10,
                0xFF,
                0x60,
                0x50,
                EZPSG_WAVE_SQUARE,
                EZPSG_PAN_CENTER
            );
        }
    }
}

// Update beat sound based on remaining asteroids
// Fewer asteroids = faster beat
void update_beat_sound(uint8_t asteroid_count) {
    // Calculate beat interval based on asteroid count
    // More asteroids = slower beat (60 frames), fewer = faster (15 frames)
    if (asteroid_count >= 8) {
        beat_interval = 60;
    } else if (asteroid_count >= 6) {
        beat_interval = 50;
    } else if (asteroid_count >= 4) {
        beat_interval = 40;
    } else if (asteroid_count >= 2) {
        beat_interval = 30;
    } else if (asteroid_count == 1) {
        beat_interval = 20;
    } else {
        beat_interval = 60;  // No asteroids, slow down
        return;  // Don't play beat if no asteroids
    }
    
    beat_timer++;
    if (beat_timer >= beat_interval) {
        beat_timer = 0;
        
        // Alternate between two bass drum-like tones
        if (beat_low) {
            ezpsg_play_note(
                c2,       // Very low
                6,        // Short
                3,        // Quick release
                0x40,     // Low duty
                0x80,     // Moderate volume
                0x40,     // Sustain
                EZPSG_WAVE_SINE,
                EZPSG_PAN_CENTER
            );
        } else {
            ezpsg_play_note(
                c3,       // Low
                6,        // Short
                3,        // Quick release
                0x40,     // Low duty
                0x80,     // Moderate volume
                0x40,     // Sustain
                EZPSG_WAVE_SINE,
                EZPSG_PAN_CENTER
            );
        }
        
        beat_low = !beat_low;
    }
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
    update_ufo_warble();
}