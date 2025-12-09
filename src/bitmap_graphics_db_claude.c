// ---------------------------------------------------------------------------
// bitmap_graphics_db.c - OPTIMIZED VERSION FOR 6502
// Minimal RAM usage - optimized for speed
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "font5x7.h"
#include "colors.h"
#include "bitmap_graphics_db.h"

// For drawing lines
static uint16_t canvas_struct = 0xFF00;
static uint16_t canvas_data = 0x0000;
static uint8_t  plane = 0;
static uint8_t  canvas_mode = 2;
static uint16_t canvas_w = 320;
static uint16_t canvas_h = 180;
static uint8_t  bpp_mode = 3;

// For drawing characters
static uint16_t cursor_y = 0;
static uint16_t cursor_x = 0;
static uint8_t textmultiplier = 1;
static uint16_t textcolor = 15;
static uint16_t textbgcolor = 15;
static bool wrap = true;

// Optimization: Store bytes per row for fast multiplication
static uint16_t bytes_per_row = 0;

// ---------------------------------------------------------------------------
static const uint8_t bpp_mode_to_bpp[] = {1, 2, 4, 8, 16};

static uint8_t bbp_to_bpp_mode(uint8_t bpp)
{
    switch(bpp) {
        case 1:  return 0;
        case 2:  return 1;
        case 4:  return 2;
        case 8:  return 3;
        case 16: return 4;
    }
    return 2;
}

void init_bitmap_graphics(uint16_t canvas_struct_address,
                          uint16_t canvas_data_address,
                          uint8_t  canvas_plane,
                          uint8_t  canvas_type,
                          uint16_t canvas_width,
                          uint16_t canvas_height,
                          uint8_t  bits_per_pixel)
{
    uint8_t x_offset = 0;
    uint8_t y_offset = 0;

    canvas_struct = 0xFF00;
    canvas_data = 0x0000;
    plane = 0;
    canvas_mode = 2;
    canvas_w = 320;
    canvas_h = 180;
    bpp_mode = 3;

    if (canvas_struct_address != 0) {
        canvas_struct = canvas_struct_address;
    }
    if (canvas_data_address != 0) {
        canvas_data = canvas_data_address;
    }
    if (canvas_plane <= 2) {
        plane = canvas_plane;
    }
    if (canvas_type > 0 && canvas_type <= 4) {
        canvas_mode = canvas_type;
    }
    if (canvas_width > 0 && canvas_width <= 640) {
        canvas_w = canvas_width;
    }
    if (canvas_height > 0 && canvas_height <= 480) {
        canvas_h = canvas_height;
    }
    if (bits_per_pixel == 1 || bits_per_pixel == 2 || bits_per_pixel == 4 ||
        bits_per_pixel == 8 || bits_per_pixel == 16) {
        bpp_mode = bbp_to_bpp_mode(bits_per_pixel);
    }

    if (bpp_mode_to_bpp[bpp_mode] == 16) {
        canvas_mode = 2;
        canvas_w = 240;
        canvas_h = 124;
    } else if (bpp_mode_to_bpp[bpp_mode] == 8) {
        canvas_mode = 2;
        canvas_w = 240;
        canvas_h = 124;
    } else if (bpp_mode_to_bpp[bpp_mode] == 4) {
        canvas_w = 320;
        if (canvas_mode > 2) {
            canvas_mode = 1;
            canvas_h = 240;
        } else if (canvas_mode == 2) {
            canvas_h = 180;
        }
    } else if (bpp_mode_to_bpp[bpp_mode] == 2) {
        if (canvas_mode == 4) {
            canvas_h = 360;
        }
    }

    if (bpp_mode_to_bpp[bpp_mode] == 16) {
        x_offset = 30;
        y_offset = 29;
    }

    xregn(1, 0, 0, 1, canvas_mode);

    xram0_struct_set(canvas_struct, vga_mode3_config_t, x_wrap, false);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, y_wrap, false);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, x_pos_px, x_offset);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, y_pos_px, y_offset);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, width_px, canvas_w);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, height_px, canvas_h);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, xram_data_ptr, canvas_data);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);

    xregn(1, 0, 1, 4, 3, bpp_mode, canvas_struct, plane);

    // Cache bytes per row for fast pixel addressing
    if (bpp_mode == 0) {
        bytes_per_row = canvas_w >> 3;
    }
}

uint16_t canvas_width(void)
{
    return canvas_w;
}

uint16_t canvas_height(void)
{
    return canvas_h;
}

uint8_t bits_per_pixel(void)
{
    return bpp_mode_to_bpp[bpp_mode];
}

void set_cursor(uint16_t x, uint16_t y)
{
    cursor_x = x;
    cursor_y = y;
}

void set_text_multiplier(uint8_t mult)
{
    textmultiplier = (mult > 0) ? mult : 1;
}

void set_text_color(uint16_t color)
{
    textcolor = textbgcolor = color;
}

void set_text_colors(uint16_t color, uint16_t background)
{
    textcolor   = color;
    textbgcolor = background;
}

void set_text_wrap(bool w)
{
    wrap = w;
}

void switch_buffer(uint16_t buffer_data_address)
{
    xram0_struct_set(canvas_struct, vga_mode3_config_t, xram_data_ptr, buffer_data_address);
}

// OPTIMIZED: Fast buffer erase
void erase_buffer(uint16_t buffer_data_address)
{
    uint16_t num_bytes;

    if (bpp_mode == 0) { // 1bpp - optimized for 640x360
        num_bytes = 28800; // 640*360/8
    } else if (bpp_mode == 1) { // 2bpp
        num_bytes = (canvas_w >> 2) * canvas_h;
    } else if (bpp_mode == 2) { // 4bpp
        num_bytes = (canvas_w >> 1) * canvas_h;
    } else if (bpp_mode == 3) { // 8bpp
        num_bytes = canvas_w * canvas_h;
    } else { // 16bpp
        num_bytes = (canvas_w << 1) * canvas_h;
    }

    RIA.addr0 = buffer_data_address;
    RIA.step0 = 1;
    
    uint16_t blocks = num_bytes >> 6;
    for (uint16_t i = 0; i < blocks; i++) {
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
        RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0; RIA.rw0 = 0;
    }
    
    uint16_t remaining = num_bytes & 0x3F;
    for (uint16_t i = 0; i < remaining; i++) {
        RIA.rw0 = 0;
    }
}

// OPTIMIZED: Fast pixel drawing for 1bpp mode
void draw_pixel2buffer(uint16_t color, uint16_t x, uint16_t y, uint16_t buffer_data_address)
{
    if (bpp_mode == 0) { // 1bpp
        if (x >= canvas_w || y >= canvas_h) return;
        
        uint8_t shift = 7 - (x & 7);
        uint16_t addr;
        
        // For 640 width: 80 bytes per row = (y<<6) + (y<<4)
        if (bytes_per_row == 80) {
            addr = buffer_data_address + ((y << 6) + (y << 4)) + (x >> 3);
        } else {
            addr = buffer_data_address + (y * bytes_per_row) + (x >> 3);
        }
        
        RIA.addr0 = addr;
        RIA.step0 = 0;
        
        if (color) {
            RIA.rw0 |= (1 << shift);
        } else {
            RIA.rw0 &= ~(1 << shift);
        }
    }
}

// OPTIMIZED: Bresenham's line algorithm
void draw_line2buffer(uint16_t color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t buffer_data_address)
{
    int16_t dx, dy, err, ystep;
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    dx = x1 - x0;
    dy = abs(y1 - y0);
    err = dx >> 1;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0 <= x1; x0++) {
        if (steep) {
            draw_pixel2buffer(color, y0, x0, buffer_data_address);
        } else {
            draw_pixel2buffer(color, x0, y0, buffer_data_address);
        }

        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void draw_char2buffer(char chr, uint16_t x, uint16_t y, uint16_t buffer_data_address)
{
    if (x >= canvas_w || y >= canvas_h) return;

    for (uint8_t i = 0; i < 6; i++) {
        uint8_t line = (i == 5) ? 0x0 : pgm_read_byte(font + (chr * 5) + i);

        for (uint8_t j = 0; j < 8; j++) {
            if (line & 0x1) {
                draw_pixel2buffer(textcolor, x + i, y + j, buffer_data_address);
            } else if (textbgcolor != textcolor) {
                draw_pixel2buffer(textbgcolor, x + i, y + j, buffer_data_address);
            }
            line >>= 1;
        }
    }
}

static void draw_char_at_cursor2buffer(char chr, uint16_t buffer_data_address)
{
    if (chr == '\n') {
        cursor_y += 8;
        cursor_x = 0;
    } else if (chr == '\r') {
        // skip
    } else {
        draw_char2buffer(chr, cursor_x, cursor_y, buffer_data_address);
        cursor_x += 6;
        if (wrap && (cursor_x > (canvas_w - 6))) {
            cursor_y += 8;
            cursor_x = 0;
        }
    }
}

void draw_string2buffer(char *str, uint16_t buffer_data_address)
{
    while (*str) {
        draw_char_at_cursor2buffer(*str++, buffer_data_address);
    }
}