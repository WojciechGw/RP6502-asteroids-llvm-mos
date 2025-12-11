/*
 * Asteroids-like sounds for RP6502 using ezpsg
 * Based on the pop-rock example by Rumbledethumps
 */

#include "ezpsg.h"
#include <rp6502.h>
#include <stdlib.h>
#include <stdio.h>

// Sound effect definitions
#define SHOOT_SOUND -1
#define EXPLOSION_SOUND -2
#define THRUST_SOUND -3

// A simple sequence to play each sound
static const uint8_t sound_effects[] = {
    SHOOT_SOUND,
    EXPLOSION_SOUND,
    THRUST_SOUND,
    0 // end of sequence
};

void ezpsg_instruments(const uint8_t **data)
{
    switch ((int8_t) * (*data)++) // instrument
    {
    case SHOOT_SOUND: // "Pew" sound for shooting
        ezpsg_play_note(c7,   // note: high C
                        1,    // duration: very short
                        5,    // release: short fade-out
                        0,    // duty: not used for sine
                        0x81, // vol_attack: instant attack, high volume
                        0xF8, // vol_decay: fast decay
                        0x10, // wave_release: Sine wave
                        0);   // pan: center
        break;
    case EXPLOSION_SOUND: // Descending arpeggio for explosion
        // Play a rapid sequence of descending notes to simulate an explosion
        for (int i = 0; i < 8; i++) {
            ezpsg_play_note(c4 - i * 4, // note: descending notes
                            1,          // duration: very short
                            2,          // release: very short
                            128,        // duty: for square wave
                            0x81 - i*0x10, // vol_attack: decreasing volume
                            0xF9,       // vol_decay: fast decay
                            0x40,       // wave_release: Square wave
                            (i % 2 == 0) ? -64 : 64); // pan: alternating left/right
        }
        break;
    case THRUST_SOUND: // Low rumble for thrust
        ezpsg_play_note(c2,   // note: low C
                        60,   // duration: long, sustained
                        10,   // release: short fade when done
                        0,    // duty: not used for sine
                        0x41, // vol_attack: soft attack, medium volume
                        0xFF, // vol_decay: sustain
                        0x10, // wave_release: Sine wave
                        0);   // pan: center
        break;
    default:
        // In a real game, you would handle unknown instruments
        break;
    }
}

void main(void)
{
    uint8_t v = RIA.vsync;

    ezpsg_init(0xFF00);

    // To play a sound, you would create a "song" on the fly
    // and call ezpsg_play_song(). For this demo, we'll just
    // play the shoot sound.
    const uint8_t shoot_sequence[] = {SHOOT_SOUND, 0};
    ezpsg_play_song(shoot_sequence);

    printf("Playing shoot sound...\n");

    while (ezpsg_playing())
    {
        if (RIA.vsync == v)
            continue;
        v = RIA.vsync;
        ezpsg_tick(3);
    }

    // Now play the explosion sound
    const uint8_t explosion_sequence[] = {EXPLOSION_SOUND, 0};
    ezpsg_play_song(explosion_sequence);
    printf("Playing explosion sound...\n");

    while (ezpsg_playing())
    {
        if (RIA.vsync == v)
            continue;
        v = RIA.vsync;
        ezpsg_tick(3);
    }

    // And the thrust sound
    const uint8_t thrust_sequence[] = {THRUST_SOUND, 0};
    ezpsg_play_song(thrust_sequence);
    printf("Playing thrust sound...\n");

    while (ezpsg_playing())
    {
        if (RIA.vsync == v)
            continue;
        v = RIA.vsync;
        ezpsg_tick(3);
    }
}
