/**
 * @file fb.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Structure containing information about the backbuffer.
 */
typedef struct {
    uint8_t* backbuffer;
    size_t backbuffer_size;
    uint32_t scroll_offset;
} backbuffer_info_t;

/**
 * @brief Structure representing a 32-bit ARGB color.
 * @param a Alpha channel.
 * @param r Red channel.
 * @param g Green channel.
 * @param b Blue channel.
 */
typedef struct {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

extern color_t black;
extern color_t white;
extern color_t red;
extern color_t green;
extern color_t blue;

void fb_update(void);
void fb_init(void);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);
size_t fb_get_backbuffer_size(void);
void fb_put_pixel(uint32_t x, uint32_t y, color_t color);
color_t fb_get_pixel(uint32_t x, uint32_t y);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);
void fb_draw_unicode(uint32_t unicode, uint32_t x, uint32_t y, color_t fg, color_t bg);
void fb_scroll(uint32_t lines, color_t color);
void fb_scroll_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t lines, color_t color);
void fb_clear(color_t color);
void fb_swap_buffers(void);
