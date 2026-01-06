#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <stddef.h>
#include "kernel.h"

void fb_init();
uint32_t fb_get_width();
uint32_t fb_get_height();
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);
void fb_scroll(uint32_t lines, uint32_t color);
void fb_clear(uint32_t color);

#endif // FB_H
