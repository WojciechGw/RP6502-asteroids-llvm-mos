#ifndef SHIP_H
#define SHIP_H

#include <stdint.h>
#include <stdbool.h>
#include "definitions.h"
#include "colors.h"


// Ship structure
typedef struct {
    int16_t x, y;           // Position
    int16_t vel_x, vel_y;    // Velocity
    uint8_t angle;           // Angle index (0-47, 7.5-degree increments)
    uint8_t size;            // Ship size (for scaling)
    uint8_t lives;               // Number of lives remaining
} Ship;

extern Ship player_ship;
extern bool show_fire;

void init_ship(void);
void apply_thrust(void);
void draw_ship(uint16_t buffer);
void relocate_ship();

#endif // SHIP_H
