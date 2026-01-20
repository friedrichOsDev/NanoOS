/*
 * @file fb.c
 * @brief Framebuffer driver implementation
 * @author friedrichOsDev
 */

#include <fb.h>
#include <font.h>
#include <heap.h>
#include <string.h>
#include <serial.h>
#include <cpu.h>
#include <kernel.h>

static fb_info_t fb_info;

/*
 * Dirty rectangle top-left corner x position
 * @note x value is in pixels
 */
static uint32_t dirty_x1;

/*
 * Dirty rectangle top-left corner y position
 * @note y value is in pixels
 */
static uint32_t dirty_y1;

/*
 * Dirty rectangle bottom-right corner x position
 * @note x value is in pixels
 */
static uint32_t dirty_x2;

/*
 * Dirty rectangle bottom-right corner y position
 * @note y value is in pixels
 */
static uint32_t dirty_y2;

/*
 * Optimized 32-bit memory set
 * @param dest Destination pointer
 * @param val 32-bit value to set
 * @param count Number of 32-bit values to set
 * @note This function is ment to be used only in the framebuffer driver (for general use there is a memset implementation in string.h)
 */
static inline void memset32(void* dest, uint32_t val, size_t count) {
    __asm__ __volatile__ ("cld; rep stosl" : "+D"(dest), "+c"(count) : "a"(val) : "memory");
}

/*
 * Optimized 32-bit memory copy
 * @param dest Destination pointer
 * @param src Source pointer
 * @param count Number of 32-bit values to copy
 * @note This function is ment to be used only in the framebuffer driver (for general use there is a memcpy implementation in string.h)
 */
static inline void memcpy32(void* dest, const void* src, size_t count) {
    __asm__ __volatile__ ("cld; rep movsl" : "+D"(dest), "+S"(src), "+c"(count) : : "memory");
}

/*
 * Marks a rectangular region of the framebuffer as dirty, meaning it needs to be redrawn.
 * @param x The x-coordinate of the top-left corner of the dirty rectangle.
 * @param y The y-coordinate of the top-left corner of the dirty rectangle.
 * @param w The width of the dirty rectangle.
 * @param h The height of the dirty rectangle.
 * @note This function is used to optimize `fb_swap_buffers` by only redrawing changed areas.
 */
static void fb_mark_dirty(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!fb_info.back_buffer) return;
    if (x >= screen_info->width || y >= screen_info->height) return;

    uint32_t x_end = x + w;
    uint32_t y_end = y + h;

    if (x_end > screen_info->width) x_end = screen_info->width;
    if (y_end > screen_info->height) y_end = screen_info->height;

    if (dirty_x2 == 0 && dirty_y2 == 0) {
        dirty_x1 = x;
        dirty_y1 = y;
        dirty_x2 = x_end;
        dirty_y2 = y_end;
    } else {
        if (x < dirty_x1) dirty_x1 = x;
        if (y < dirty_y1) dirty_y1 = y;
        if (x_end > dirty_x2) dirty_x2 = x_end;
        if (y_end > dirty_y2) dirty_y2 = y_end;
    }
}

/*
 * A funnction to initialize the framebuffer
 * @param void
 */
void fb_init(void) {
    size_t buffer_size = screen_info->height * screen_info->bytes_per_line;
    uint32_t scroll_offset = 0;

    fb_info.back_buffer = kmalloc(buffer_size);

    if (!fb_info.back_buffer) {
        serial_puts("fb_init: failed to allocate back buffer\n");
        return;
    }
    fb_info.back_buffer_size = buffer_size;
    fb_info.scroll_offset = scroll_offset;

    uint32_t pot_size = 1;
    while (pot_size < buffer_size) {
        pot_size <<= 1;
    }

    cpu_enable_write_combining(screen_info->physical_buffer, pot_size);

    dirty_x1 = dirty_y1 = dirty_x2 = dirty_y2 = 0;
    
    fb_clear(0x000000);
}

/*
 * A function to get the framebuffer information
 * @param void
 * @return A pointer to the framebuffer information structure
 */
fb_info_t* fb_get_info(void) {
    return &fb_info;
}

/*
 * A function to get the width of the framebuffer
 * @param void
 * @return The width of the framebuffer in pixels
 */
uint32_t fb_get_width(void) {
    return screen_info->width;
}

/*
 * A function to get the height of the framebuffer
 * @param void
 * @return The height of the framebuffer in pixels
 */
uint32_t fb_get_height(void) {
    return screen_info->height;
}

/*
 * A function to get the size of the back buffer
 * @param void
 * @return The size of the back buffer in bytes
 */
size_t fb_get_back_buffer_size(void) {
    return fb_info.back_buffer_size;
}

/*
 * A function to put a pixel on the screen
 * @param x The x-coordinate of the pixel
 * @param y The y-coordinate of the pixel
 * @param color The color of the pixel
 */
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;
    if (x >= screen_info->width || y >= screen_info->height) return;

    fb_mark_dirty(x, y, 1, 1);

    uint32_t offset = (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel) + fb_info.scroll_offset;
    if (offset >= fb_info.back_buffer_size) offset -= fb_info.back_buffer_size;

    *((uint32_t*)(fb_info.back_buffer + offset)) = color;
}

/*
 * A function to get a pixel from the screen
 * @param x The x-coordinate of the pixel
 * @param y The y-coordinate of the pixel
 * @return The color of the pixel
 */
uint32_t fb_get_pixel(uint32_t x, uint32_t y) {
    if (!fb_info.back_buffer) return 0;
    if (screen_info->bytes_per_pixel != 4) return 0;
    if (x >= screen_info->width || y >= screen_info->height) return 0;

    uint32_t offset = (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel) + fb_info.scroll_offset;
    if (offset >= fb_info.back_buffer_size) offset -= fb_info.back_buffer_size;

    return *((uint32_t*)(fb_info.back_buffer + offset));
}

/*
 * A function to draw a rectangle on the screen
 * @param x The x-coordinate of the top-left corner of the rectangle
 * @param y The y-coordinate of the top-left corner of the rectangle
 * @param width The width of the rectangle
 * @param height The height of the rectangle
 * @param color The color of the rectangle
 */
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;
    if (x >= screen_info->width || y >= screen_info->height) return;

    if (x + width > screen_info->width) width = screen_info->width - x;
    if (y + height > screen_info->height) height = screen_info->height - y;
    
    fb_mark_dirty(x, y, width, height);
    
    for (uint32_t j = 0; j < height; j++) {
        uint32_t row = y + j;
        if (row >= screen_info->height) continue;

        uint32_t offset = (row * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel) + fb_info.scroll_offset;
        if (offset >= fb_info.back_buffer_size) {
            offset -= fb_info.back_buffer_size;
        }
        
        uint32_t draw_width = width;
        if (x + draw_width > screen_info->width) draw_width = screen_info->width - x;

        uint32_t* dest = (uint32_t*)(fb_info.back_buffer + offset);
        memset32(dest, color, draw_width);
    }
}

/*
 * A function to draw a character on the screen
 * @param x The x-coordinate of the top-left corner of the character
 * @param y The y-coordinate of the top-left corner of the character
 * @param c The character to draw
 * @param fg_color The foreground color of the character
 * @param bg_color The background color of the character
 */
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;
    uint8_t* font_char = font8x8_basic[(uint8_t)c];
    if (!font_char) return;

    fb_mark_dirty(x, y, FONT_WIDTH, FONT_HEIGHT);

    uint32_t offset = (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel) + fb_info.scroll_offset;
    if (offset >= fb_info.back_buffer_size) offset -= fb_info.back_buffer_size;

    uint32_t* dest = (uint32_t*)(fb_info.back_buffer + offset);
    uint32_t line_offset = screen_info->bytes_per_line / 4;
    uint32_t* buffer_start = (uint32_t*)fb_info.back_buffer;
    uint32_t* buffer_end = (uint32_t*)(fb_info.back_buffer + fb_info.back_buffer_size);

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            dest[j] = ((font_char[i] >> j) & 0x01) ? fg_color : bg_color;
        }

        dest += line_offset;

        if (dest >= buffer_end) {
            dest = buffer_start + (dest - buffer_end);
        }
    }
}

/*
 * A function to scroll the screen up by a number of lines
 * @param lines The number of lines to scroll up
 * @param color The color to fill the new lines with
 */
void fb_scroll(uint32_t lines, uint32_t color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;
    if (lines == 0) return;
    if (lines >= screen_info->height) {
        fb_clear(color);
        return;
    }

    fb_mark_dirty(0, 0, screen_info->width, screen_info->height);

    uint32_t bytes_to_scroll = lines * screen_info->bytes_per_line;
    uint32_t old_offset = fb_info.scroll_offset;

    fb_info.scroll_offset += bytes_to_scroll;
    if (fb_info.scroll_offset >= fb_info.back_buffer_size) fb_info.scroll_offset -= fb_info.back_buffer_size;

    if (old_offset + bytes_to_scroll > fb_info.back_buffer_size) {
        uint32_t part1_len = fb_info.back_buffer_size - old_offset;
        uint32_t part2_len = bytes_to_scroll - part1_len;
        memset32(fb_info.back_buffer + old_offset, color, part1_len / 4);
        memset32(fb_info.back_buffer, color, part2_len / 4);
    } else {
        memset32(fb_info.back_buffer + old_offset, color, bytes_to_scroll / 4);
    }
}

/*
 * A function to clear the screen
 * @param color The color to clear the screen with
 */
void fb_clear(uint32_t color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;

    uint32_t* dest = (uint32_t*)fb_info.back_buffer;
    size_t num_pixels = fb_info.back_buffer_size / 4;
    memset32(dest, color, num_pixels);
    fb_mark_dirty(0, 0, screen_info->width, screen_info->height);
}

/*
 * A function to swap the back buffer with the actual framebuffer
 * @param void
 */
void fb_swap_buffers(void) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;

    if (dirty_x2 == 0 && dirty_y2 == 0) return;

    uint32_t* vram = (uint32_t*)(screen_info->physical_buffer);
    
    for (uint32_t y = dirty_y1; y < dirty_y2; y++) {
        uint32_t offset = (y * screen_info->bytes_per_line) + (dirty_x1 * screen_info->bytes_per_pixel);
        
        uint32_t src_offset = offset + fb_info.scroll_offset;
        if (src_offset >= fb_info.back_buffer_size) {
            src_offset -= fb_info.back_buffer_size;
        }

        uint32_t* dest = (uint32_t*)((uint8_t*)vram + offset);
        uint32_t* src = (uint32_t*)(fb_info.back_buffer + src_offset);
        uint32_t width = dirty_x2 - dirty_x1;
        memcpy32(dest, src, width);
    }

    dirty_x1 = dirty_y1 = dirty_x2 = dirty_y2 = 0;

    cpu_mfence();
}