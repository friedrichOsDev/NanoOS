#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <stddef.h>
#include <kernel.h>

typedef struct {
    uint8_t* back_buffer;
    size_t back_buffer_size;
    uint32_t scroll_offset;
} fb_info_t;

void fb_init();
uint32_t fb_get_width();
uint32_t fb_get_height();
size_t fb_get_back_buffer_size();
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);
void fb_scroll(uint32_t lines, uint32_t color);
void fb_clear(uint32_t color);
void fb_swap_buffers();

#endif // FB_H
