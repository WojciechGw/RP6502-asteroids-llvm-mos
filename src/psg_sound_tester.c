/*
 * PSG Sound Tester for Picocomputer RP6502
 * Interactive parameter editor and sound player
 */

#include "ezpsg.h"
#include <rp6502.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// Parameter indices
#define PARAM_NOTE 0
#define PARAM_DURATION 1
#define PARAM_RELEASE 2
#define PARAM_DUTY 3
#define PARAM_VOL_ATTACK 4
#define PARAM_VOL_DECAY 5
#define PARAM_WAVE_RELEASE 6
#define PARAM_PAN 7
#define PARAM_COUNT 8

// Waveform constants
#define EZPSG_WAVE_SINE 0x00
#define EZPSG_WAVE_SQUARE 0x10
#define EZPSG_WAVE_SAW 0x20
#define EZPSG_WAVE_TRI 0x30
#define EZPSG_WAVE_NOISE 0x40

// Keyboard handling
#define KEYBOARD_INPUT 0xFF10
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

// USB HID key codes we need
#define KEY_UP 0x52
#define KEY_DOWN 0x51
#define KEY_LEFT 0x50
#define KEY_RIGHT 0x4F
#define KEY_SPACE 0x2C
#define KEY_Q 0x14
#define KEY_W 0x1A
#define KEY_R 0x15
#define KEY_EQUALS 0x2E
#define KEY_MINUS 0x2D
#define KEY_LEFT_BRACKET 0x2F
#define KEY_RIGHT_BRACKET 0x30

// Current parameter values
static struct {
    uint8_t note;
    uint8_t duration;
    uint8_t release;
    uint8_t duty;
    uint8_t vol_attack;
    uint8_t vol_decay;
    uint8_t wave_release;
    int8_t pan;
} params = {
    .note = c4,           // Middle C (c4)
    .duration = 10,
    .release = 5,
    .duty = 128,
    .vol_attack = 0x01,   // Fast attack, full volume
    .vol_decay = 0xF5,    // Slow decay, high sustain
    .wave_release = EZPSG_WAVE_SQUARE | 0x05,
    .pan = 0
};

static int current_param = 0;

// Note names for display
static const char* note_names[] = {
    "A0","As0","B0",
    "C1","Cs1","D1","Ds1","E1","F1","Fs1","G1","Gs1","A1","As1","B1",
    "C2","Cs2","D2","Ds2","E2","F2","Fs2","G2","Gs2","A2","As2","B2",
    "C3","Cs3","D3","Ds3","E3","F3","Fs3","G3","Gs3","A3","As3","B3",
    "C4","Cs4","D4","Ds4","E4","F4","Fs4","G4","Gs4","A4","As4","B4",
    "C5","Cs5","D5","Ds5","E5","F5","Fs5","G5","Gs5","A5","As5","B5",
    "C6","Cs6","D6","Ds6","E6","F6","Fs6","G6","Gs6","A6","As6","B6",
    "C7","Cs7","D7","Ds7","E7","F7","Fs7","G7","Gs7","A7","As7","B7",
    "C8"
};

// static const char* note_names[] = {
//     "C0", "Cs0", "D0", "Ds0", "E0", "F0", "Fs0", "G0", "Gs0", "A0", "As0", "B0",
//     "C1", "Cs1", "D1", "Ds1", "E1", "F1", "Fs1", "G1", "Gs1", "A1", "As1", "B1",
//     "C2", "Cs2", "D2", "Ds2", "E2", "F2", "Fs2", "G2", "Gs2", "A2", "As2", "B2",
//     "C3", "Cs3", "D3", "Ds3", "E3", "F3", "Fs3", "G3", "Gs3", "A3", "As3", "B3",
//     "C4", "Cs4", "D4", "Ds4", "E4", "F4", "Fs4", "G4", "Gs4", "A4", "As4", "B4",
//     "C5", "Cs5", "D5", "Ds5", "E5", "F5", "Fs5", "G5", "Gs5", "A5", "As5", "B5",
//     "C6", "Cs6", "D6", "Ds6", "E6", "F6", "Fs6", "G6", "Gs6", "A6", "As6", "B6",
//     "C7", "Cs7", "D7", "Ds7", "E7", "F7", "Fs7", "G7", "Gs7", "A7", "As7", "B7",
//     "C8", "Cs8", "D8", "Ds8", "E8", "F8", "Fs8", "G8", "Gs8", "A8", "As8", "B8"
// };

static const char* waveform_names[] = {
    "Sine", "Square", "Sawtooth", "Triangle", "Noise"
};

// Get waveform name from wave_release value
static const char* get_waveform_name(uint8_t wave_release) {
    uint8_t wave = (wave_release >> 4) & 0x07;
    if (wave < 5) return waveform_names[wave];
    return "Unknown";
}

// Clear screen (simple approach)
static void clear_screen(void) {
    printf("\033[2J\033[H");
}

// Display current parameters
static void display_params(void) {
    clear_screen();
    printf("========================================\n");
    printf("    PSG SOUND TESTER for RP6502\n");
    printf("========================================\n\n");
    
    // Extract components
    uint8_t vol_attack_vol = (params.vol_attack >> 4) & 0x0F;
    uint8_t vol_attack_rate = params.vol_attack & 0x0F;
    uint8_t vol_decay_vol = (params.vol_decay >> 4) & 0x0F;
    uint8_t vol_decay_rate = params.vol_decay & 0x0F;
    uint8_t waveform = (params.wave_release >> 4) & 0x07;
    uint8_t release_rate = params.wave_release & 0x0F;
    
    printf("%c Note:            %3d (%s)\n", 
           current_param == PARAM_NOTE ? '>' : ' ',
           params.note, 
           params.note < 88 ? note_names[params.note] : "???");
    
    printf("%c Duration:        %3d ticks\n",
           current_param == PARAM_DURATION ? '>' : ' ',
           params.duration);
    
    printf("%c Release:         %3d ticks\n",
           current_param == PARAM_RELEASE ? '>' : ' ',
           params.release);
    
    printf("%c Duty Cycle:      %3d (%%%d)\n",
           current_param == PARAM_DUTY ? '>' : ' ',
           params.duty,
           (params.duty * 100) / 255);
    
    printf("%c Vol/Attack:    0x%02X (vol:%d att:%d)\n",
           current_param == PARAM_VOL_ATTACK ? '>' : ' ',
           params.vol_attack,
           vol_attack_vol,
           vol_attack_rate);
    
    printf("%c Vol/Decay:     0x%02X (vol:%d dec:%d)\n",
           current_param == PARAM_VOL_DECAY ? '>' : ' ',
           params.vol_decay,
           vol_decay_vol,
           vol_decay_rate);
    
    printf("%c Wave/Release:  0x%02X (%s, rel:%d)\n",
           current_param == PARAM_WAVE_RELEASE ? '>' : ' ',
           params.wave_release,
           get_waveform_name(params.wave_release),
           release_rate);
    
    printf("%c Pan:           %+4d (%s)\n",
           current_param == PARAM_PAN ? '>' : ' ',
           params.pan,
           params.pan < -20 ? "Left" : params.pan > 20 ? "Right" : "Center");
    
    printf("\n========================================\n");
    printf("Controls:\n");
    printf("  Up/Down  - Select parameter\n");
    printf("  +/-      - Adjust value (+/-1)\n");
    printf("  [/]      - Adjust value (+/-10)\n");
    printf("  Space    - Play sound\n");
    printf("  W        - Cycle waveform\n");
    printf("  R        - Reset to defaults\n");
    printf("  Q        - Quit\n");
    printf("========================================\n");
}

// Read keyboard state
static void read_keyboard(void) {
    xregn(0, 0, 0, 1, KEYBOARD_INPUT);
    for (uint8_t i = 0; i < KEYBOARD_BYTES; i++) {
        RIA.addr0 = KEYBOARD_INPUT + i;
        keystates[i] = RIA.rw0;
    }
}

// Play the current sound
static void play_sound(void) {
    printf("\nPlaying sound...\n");
    ezpsg_play_note(params.note,
                    params.duration,
                    params.release,
                    params.duty,
                    params.vol_attack,
                    params.vol_decay,
                    params.wave_release,
                    params.pan);
}

// Adjust parameter value
static void adjust_param(int delta) {
    switch (current_param) {
        case PARAM_NOTE:
            if (delta > 0 && params.note < 87) params.note += delta;
            else if (delta < 0 && params.note > 0) {
                params.note = (params.note >= -delta) ? params.note + delta : 0;
            }
            break;
            
        case PARAM_DURATION:
            if (delta > 0 && params.duration < 255) {
                params.duration = (params.duration + delta > 255) ? 255 : params.duration + delta;
            }
            else if (delta < 0 && params.duration > 1) {
                params.duration = (params.duration >= -delta) ? params.duration + delta : 1;
            }
            break;
            
        case PARAM_RELEASE:
            if (delta > 0 && params.release < 255) {
                params.release = (params.release + delta > 255) ? 255 : params.release + delta;
            }
            else if (delta < 0 && params.release > 0) {
                params.release = (params.release >= -delta) ? params.release + delta : 0;
            }
            break;
            
        case PARAM_DUTY:
            if (delta > 0 && params.duty < 255) {
                params.duty = (params.duty + delta > 255) ? 255 : params.duty + delta;
            }
            else if (delta < 0 && params.duty > 0) {
                params.duty = (params.duty >= -delta) ? params.duty + delta : 0;
            }
            break;
            
        case PARAM_VOL_ATTACK:
        case PARAM_VOL_DECAY:
        case PARAM_WAVE_RELEASE:
            {
                uint8_t *param = (current_param == PARAM_VOL_ATTACK) ? &params.vol_attack :
                                (current_param == PARAM_VOL_DECAY) ? &params.vol_decay :
                                &params.wave_release;
                if (delta > 0 && *param < 255) {
                    *param = (*param + delta > 255) ? 255 : *param + delta;
                }
                else if (delta < 0 && *param > 0) {
                    *param = (*param >= -delta) ? *param + delta : 0;
                }
            }
            break;
            
        case PARAM_PAN:
            if (delta > 0 && params.pan < 63) {
                params.pan = (params.pan + delta > 63) ? 63 : params.pan + delta;
            }
            else if (delta < 0 && params.pan > -63) {
                params.pan = (params.pan >= -delta) ? params.pan + delta : -63;
            }
            break;
    }
}

// Cycle through waveforms
static void cycle_waveform(void) {
    uint8_t current_wave = (params.wave_release >> 4) & 0x07;
    uint8_t release_rate = params.wave_release & 0x0F;
    
    current_wave = (current_wave + 1) % 5;
    params.wave_release = (current_wave << 4) | release_rate;
}

// Reset to defaults
static void reset_params(void) {
    params.note = 60;
    params.duration = 10;
    params.release = 5;
    params.duty = 128;
    params.vol_attack = 0x01;
    params.vol_decay = 0xF5;
    params.wave_release = EZPSG_WAVE_SQUARE | 0x05;
    params.pan = 0;
}

// Main function
int main(void) {
    uint8_t v;
    bool running = true;
    bool key_was_pressed[8] = {false}; // Track key states to prevent repeat
    
    // Initialize PSG
    ezpsg_init(0xFF00);
    
    display_params();
    
    // vsync loop
    v = RIA.vsync;
    
    while (running) {
        // Wait for vsync
        if (v == RIA.vsync) {
            continue;
        }
        v = RIA.vsync;
        
        // Update PSG tick
        ezpsg_tick(1);
        
        // Read keyboard
        read_keyboard();
        
        // Handle Up arrow - previous parameter
        if (key(KEY_UP)) {
            if (!key_was_pressed[0]) {
                current_param = (current_param - 1 + PARAM_COUNT) % PARAM_COUNT;
                display_params();
                key_was_pressed[0] = true;
            }
        } else {
            key_was_pressed[0] = false;
        }
        
        // Handle Down arrow - next parameter
        if (key(KEY_DOWN)) {
            if (!key_was_pressed[1]) {
                current_param = (current_param + 1) % PARAM_COUNT;
                display_params();
                key_was_pressed[1] = true;
            }
        } else {
            key_was_pressed[1] = false;
        }
        
        // Handle + (equals key) - increment by 1
        if (key(KEY_EQUALS) || key(KEY_RIGHT)) {
            if (!key_was_pressed[2]) {
                adjust_param(1);
                display_params();
                key_was_pressed[2] = true;
            }
        } else {
            key_was_pressed[2] = false;
        }
        
        // Handle - (minus key) - decrement by 1
        if (key(KEY_MINUS) || key(KEY_LEFT)) {
            if (!key_was_pressed[3]) {
                adjust_param(-1);
                display_params();
                key_was_pressed[3] = true;
            }
        } else {
            key_was_pressed[3] = false;
        }
        
        // Handle [ - decrement by 10
        if (key(KEY_LEFT_BRACKET)) {
            if (!key_was_pressed[4]) {
                adjust_param(-10);
                display_params();
                key_was_pressed[4] = true;
            }
        } else {
            key_was_pressed[4] = false;
        }
        
        // Handle ] - increment by 10
        if (key(KEY_RIGHT_BRACKET)) {
            if (!key_was_pressed[5]) {
                adjust_param(10);
                display_params();
                key_was_pressed[5] = true;
            }
        } else {
            key_was_pressed[5] = false;
        }
        
        // Handle Space - play sound
        if (key(KEY_SPACE)) {
            if (!key_was_pressed[6]) {
                play_sound();
                key_was_pressed[6] = true;
            }
        } else {
            key_was_pressed[6] = false;
        }
        
        // Handle W - cycle waveform
        if (key(KEY_W)) {
            if (!key_was_pressed[7]) {
                cycle_waveform();
                display_params();
                key_was_pressed[7] = true;
            }
        } else {
            key_was_pressed[7] = false;
        }
        
        // Handle R - reset (allow repeat)
        if (key(KEY_R)) {
            reset_params();
            display_params();
        }
        
        // Handle Q - quit (allow repeat)
        if (key(KEY_Q)) {
            running = false;
        }
    }
    
    clear_screen();
    printf("PSG Sound Tester exited.\n");
}