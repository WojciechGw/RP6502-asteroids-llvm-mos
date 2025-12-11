// Optimize for size to fit in Release builds
#pragma GCC optimize ("Os")

#include <rp6502.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "font5x7.h"
#include "bitmap_graphics_db.h"

// ---------------------------------------------------------------------------
// Optimization: Precomputed row offsets to avoid runtime multiplication
// 640 width / 8 bits = 80 bytes per row.
// ---------------------------------------------------------------------------
static uint16_t row_offsets[360];
static void precompute_row_offsets() {
    uint16_t addr = 0;
    int i;
    for(i = 0; i < 360; i++) {
        row_offsets[i] = addr;
        addr += 80; 
    }
}

// Global state
static uint16_t canvas_struct_addr;
static uint16_t canvas_w = 640;
static uint16_t canvas_h = 360;
// Default to 1bpp logic
static uint8_t bpp_mode = 0; 

uint16_t random(uint16_t low_limit, uint16_t high_limit)
{
    if (low_limit > high_limit) {
        swap(low_limit, high_limit);
    }

    return (uint16_t)((rand() % (high_limit-low_limit)) + low_limit);
}

void init_bitmap_graphics(uint16_t canvas_struct_address,
                          uint16_t canvas_data_address,
                          uint8_t  canvas_plane,
                          uint8_t  canvas_type,
                          uint16_t canvas_width,
                          uint16_t canvas_height,
                          uint8_t  bits_per_pixel)
{
    // Initialize lookup table
    precompute_row_offsets();

    canvas_struct_addr = canvas_struct_address;
    canvas_w = canvas_width;
    canvas_h = canvas_height;
    
    // Convert bits_per_pixel to mode
    if (bits_per_pixel == 1) bpp_mode = 0;
    else if (bits_per_pixel == 2) bpp_mode = 1;
    else if (bits_per_pixel == 4) bpp_mode = 2;
    else if (bits_per_pixel == 8) bpp_mode = 3;
    else bpp_mode = 4; // 16 bit

    // Initialize Video Hardware
    xregn(1, 0, 0, 1, canvas_type); // Canvas mode

    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, x_wrap, false);
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, y_wrap, false);
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, x_pos_px, 0);
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, y_pos_px, 0);
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, width_px, canvas_w);
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, height_px, canvas_h);
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, xram_data_ptr, canvas_data_address);
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);

    // Set Video Mode
    xregn(1, 0, 1, 4, 3, bpp_mode, canvas_struct_addr, canvas_plane);
}

void switch_buffer(uint16_t buffer_data_address)
{
    xram0_struct_set(canvas_struct_addr, vga_mode3_config_t, xram_data_ptr, buffer_data_address);
}

void erase_buffer(uint16_t buffer_data_address) __attribute__((noinline));
void draw_pixel2buffer(uint16_t color, uint16_t x, uint16_t y, uint16_t buffer_data_address) __attribute__((noinline));
void draw_line2buffer(uint16_t color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t buffer_data_address) __attribute__((noinline));


// ---------------------------------------------------------------------------
// Optimized Erase
// ---------------------------------------------------------------------------
void erase_buffer(uint16_t buffer_data_address)
{
    // 640x360 @ 1bpp = 28,800 bytes
    // We assume 1bpp for this specific optimized clone
    uint16_t total_bytes = 28800; 
    uint16_t i;
    
    RIA.addr0 = buffer_data_address;
    RIA.step0 = 1; // Auto-increment

    // Heavily unrolled loop
    for (i = 0; i < (total_bytes >> 5); i++) { // Divide by 32
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
    }
}

void draw_pixel2buffer(uint16_t color, uint16_t x, uint16_t y, uint16_t buffer_data_address)
{
    // Bounds check
    if (x >= canvas_w || y >= canvas_h) return;

    // 1. Calculate Byte Address
    // address = buffer_base + row_offset[y] + (x / 8)
    uint16_t addr = buffer_data_address + row_offsets[y] + (x >> 3);

    // 2. Calculate Bitmask
    // bitmask = 128 >> (x % 8)
    uint8_t bitmask = 0x80 >> (x & 7);

    // 3. Hardware Read-Modify-Write
    RIA.addr0 = addr;
    RIA.step0 = 0;     // Turn off auto-increment
    
    uint8_t val = RIA.rw0; // Read current byte
    
    if (color) {
        val |= bitmask;    // Set bit
    } else {
        val &= ~bitmask;   // Clear bit
    }

    RIA.addr0 = addr;  // Reset address (safeguard)
    RIA.rw0 = val;     // Write modified byte
}

// ---------------------------------------------------------------------------
// Specialized 1BPP Line Drawer (Bresenham)
// Optimizations: 
// 1. No function calls per pixel
// 2. Uses row_offsets table
// 3. Calculates bitmasks directly
// ---------------------------------------------------------------------------
void draw_line2buffer(uint16_t color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t buffer_addr)
{
    int16_t dx, dy, sx, sy, err, e2;
    uint16_t addr;
    uint8_t bitmask, val;

    // Bounds check (Simple clipping)
    if (x0 < 0) x0 = 0; if (x0 >= canvas_w) x0 = canvas_w - 1;
    if (y0 < 0) y0 = 0; if (y0 >= canvas_h) y0 = canvas_h - 1;
    if (x1 < 0) x1 = 0; if (x1 >= canvas_w) x1 = canvas_w - 1;
    if (y1 < 0) y1 = 0; if (y1 >= canvas_h) y1 = canvas_h - 1;

    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1; 
    err = (dx > dy ? dx : -dy) / 2;

    RIA.step0 = 0; // No auto-inc, we jump around

    while(true) {
        // --- DRAW PIXEL MACRO START ---
        // Address = Buffer + (y * 80) + (x / 8)
        addr = buffer_addr + row_offsets[y0] + (x0 >> 3);
        
        // Bitmask = 1 << (7 - (x % 8))  =>  0x80 >> (x & 7)
        bitmask = 0x80 >> (x0 & 7);
        
        RIA.addr0 = addr;
        val = RIA.rw0; // Read current byte
        
        if (color) val |= bitmask;
        else       val &= ~bitmask;
        
        RIA.addr0 = addr; // Reset addr (read increments it? No, step is 0)
        // Actually step=0 means address doesn't move, but we must write back
        RIA.rw0 = val;
        // --- DRAW PIXEL MACRO END ---

        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy)  { err += dx; y0 += sy; }
    }
}

// ---------------------------------------------------------------------------
// Text Drawing (Unchanged from original logic, just simplified deps)
// ---------------------------------------------------------------------------
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

void set_cursor(uint16_t x, uint16_t y) { cursor_x = x; cursor_y = y; }

void draw_char2buffer(char chr, uint16_t buffer_addr) {
    int i, j;
    uint8_t line;
    uint16_t addr_base;
    uint8_t val;
    
    if (cursor_x >= canvas_w || cursor_y >= canvas_h) return;

    for (i=0; i<5; i++) {
        line = pgm_read_byte(font + (chr * 5) + i);
        for (j=0; j<8; j++) {
            if (line & 0x01) {
                // Inline pixel draw for text
                uint16_t cx = cursor_x + i;
                uint16_t cy = cursor_y + j;
                if(cx < canvas_w && cy < canvas_h) {
                    addr_base = buffer_addr + row_offsets[cy] + (cx >> 3);
                    RIA.addr0 = addr_base;
                    RIA.step0 = 0;
                    val = RIA.rw0 | (0x80 >> (cx & 7));
                    RIA.addr0 = addr_base;
                    RIA.rw0 = val;
                }
            }
            line >>= 1;
        }
    }
}

void draw_string2buffer(char * str, uint16_t buffer_addr) {
    while (*str) {
        if (*str == '\n') {
            cursor_y += 8;
            cursor_x = 0;
        } else {
            draw_char2buffer(*str, buffer_addr);
            cursor_x += 6;
        }
        str++;
    }
}