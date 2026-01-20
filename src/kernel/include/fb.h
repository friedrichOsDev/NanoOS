/*
 * @file fb.h
 * @brief Header file for framebuffer driver implementation
 * @author friedrichOsDev
 */

#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <stddef.h>

/*
 * Back buffer information structure
 * @note Contains information about the back buffer used for off-screen rendering
 */
typedef struct {
    uint8_t* back_buffer;
    size_t back_buffer_size;
    uint32_t scroll_offset;
} fb_info_t;

void fb_init(void);
fb_info_t* fb_get_info(void);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);
size_t fb_get_back_buffer_size(void);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t fb_get_pixel(uint32_t x, uint32_t y);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);
void fb_scroll(uint32_t lines, uint32_t color);
void fb_clear(uint32_t color);
void fb_swap_buffers(void);

#endif // FB_H
