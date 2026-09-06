/**
 * @file framebuffer.c
 * @brief Framebuffer driver implementation
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAKE_COLOR(r_val, g_val, b_val)                                        \
    ((color_t){.b = (b_val), .g = (g_val), .r = (r_val), .a = 255})

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} __attribute__((packed)) color_t;

extern color_t black;
extern color_t gray;
extern color_t white;
extern color_t red;
extern color_t green;
extern color_t blue;

uint64_t fb_get_width();
uint64_t fb_get_height();
size_t fb_get_backbuffer_size();

void fb_clear(color_t color);
void fb_draw_pixel(uint64_t x, uint64_t y, color_t color);
void fb_draw_rectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                       color_t color, bool filled, uint64_t border_size);
void fb_draw_circle(uint64_t x, uint64_t y, uint64_t radius, color_t color,
                    bool filled, uint64_t border_size);
void fb_draw_triangle(uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2,
                      uint64_t x3, uint64_t y3, color_t color, bool filled,
                      uint64_t border_size);
void fb_draw_line(int64_t x1, int64_t y1, int64_t x2, int64_t y2, color_t color,
                  uint64_t thickness);

void fb_swap_buffers();
void framebuffer_init_thread(void *arg);