/**
 * @file framebuffer.h
 * @brief Simple Framebuffer Driver
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *backbuffer;
    size_t backbuffer_size;
} backbuffer_info_t;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} color_t;

extern color_t black;
extern color_t white;
extern color_t red;
extern color_t green;
extern color_t blue;

void framebuffer_init_thread();

uint64_t fb_get_width();
uint64_t fb_get_height();
size_t fb_get_backbuffer_size();

void fb_clear(color_t color);
void fb_draw_pixel(uint64_t x, uint64_t y, color_t color);
void fb_draw_rectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height, color_t color, bool filled, uint64_t border_size);
void fb_draw_circle(uint64_t x, uint64_t y, uint64_t radius, color_t color, bool filled, uint64_t border_size);
void fb_draw_triangle(uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2, uint64_t x3, uint64_t y3, color_t color, bool filled, uint64_t border_size);
void fb_draw_unicode(uint64_t x, uint64_t y, uint32_t unicode, color_t color);
void fb_draw_ustring(uint64_t x, uint64_t y, uint32_t *str, color_t color);
void fb_draw_char(uint64_t x, uint64_t y, char c, color_t color);
void fb_draw_string(uint64_t x, uint64_t y, const char *str, color_t color);
void fb_swap_buffers();