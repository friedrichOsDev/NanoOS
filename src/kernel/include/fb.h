#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <stddef.h>
#include "kernel.h"

void fb_init();
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_clear(uint32_t color);

#endif // FB_H
