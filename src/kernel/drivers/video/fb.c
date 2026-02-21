/*
 * @file fb.c
 * @brief framebuffer driver
 * @author friedrichOsDev
 */

#include <fb.h>
#include <kernel.h>
#include <serial.h>
#include <font.h>
#include <string.h>
#include <heap.h>

static backbuffer_info_t bb_info;
static uint32_t dirty_x1;
static uint32_t dirty_y1;
static uint32_t dirty_x2;
static uint32_t dirty_y2;
color_t black = {255, 0, 0, 0};
color_t white = {255, 255, 255, 255};
color_t red = {255, 255, 0, 0};
color_t green = {255, 0, 255, 0};
color_t blue = {255, 0, 0, 255};

/*
 * Optimized memset for 32-bit values using rep stosl instruction
 * @param dest destination pointer
 * @param value 32-bit value to set
 * @param count number of 32-bit values to set
 */
static inline void memset32(void* dest, uint32_t value, size_t count) {
    __asm__ __volatile__(
        "rep stosl"
        : "+D" (dest), "+c" (count)
        : "a" (value)
        : "memory"
    );
}

/*
 * Optimized memcpy for 32-bit values using rep movsl instruction
 * @param dest destination pointer
 * @param src source pointer
 * @param count number of 32-bit values to copy
 */
static inline void memcpy32(void* dest, const void* src, size_t count) {
    __asm__ __volatile__(
        "rep movsl"
        : "+D" (dest), "+S" (src), "+c" (count)
        :
        : "memory"
    );
}

/*
 * Marks a region of the framebuffer as dirty, so it will be updated on the next swap
 * @param x top-left x coordinate of the dirty region
 * @param y top-left y coordinate of the dirty region
 * @param w width of the dirty region
 * @param h height of the dirty region
 */
static void fb_mark_dirty(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (x < dirty_x1) dirty_x1 = x;
    if (y < dirty_y1) dirty_y1 = y;
    if (x + w > dirty_x2) dirty_x2 = x + w;
    if (y + h > dirty_y2) dirty_y2 = y + h;
    if (dirty_x2 > fb_get_width()) dirty_x2 = fb_get_width();
    if (dirty_y2 > fb_get_height()) dirty_y2 = fb_get_height();
}

/*
 * Initializes the framebuffer driver, sets up backbuffer and dirty region
 * @param void
 */
void fb_init(void) {
    serial_printf("FB: start\n");
    size_t buffer_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
    uint32_t scroll_offset = 0;
    serial_printf("FB: Initializing framebuffer: %dx%d, %d bpp, pitch: %d, buffer size: %d bytes\n", kernel_fb_info.fb_width, kernel_fb_info.fb_height, kernel_fb_info.fb_bpp, kernel_fb_info.fb_pitch, buffer_size);
    bb_info.backbuffer = (uint8_t*)kzalloc(buffer_size);
    serial_printf("FB: Backbuffer Virt: %x, Phys: %x\n", 
              bb_info.backbuffer, 
              vmm_virtual_to_physical(vmm_get_page_directory(), (virt_addr_t)bb_info.backbuffer));
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Failed to allocate backbuffer\n");
        return;
    }
    bb_info.backbuffer_size = buffer_size;
    bb_info.scroll_offset = scroll_offset;
    
    dirty_x1 = dirty_y1 = dirty_x2 = dirty_y2 = 0;

    serial_printf("FB: clear screen\n");
    fb_clear(black);
    fb_swap_buffers();
    serial_printf("FB: done");
}

/*
 * Get the width of the framebuffer
 * @param void
 * @return The width of the framebuffer in pixels
 */
uint32_t fb_get_width(void) {
    return kernel_fb_info.fb_width;
}

/*
 * Get the height of the framebuffer
 * @param void
 * @return The height of the framebuffer in pixels
 */
uint32_t fb_get_height(void) {
    return kernel_fb_info.fb_height;
}

/*
 * Get the size of the backbuffer
 * @param void
 * @return The size of the backbuffer in bytes
 */
size_t fb_get_backbuffer_size(void) {
    return bb_info.backbuffer_size;
}

/*
 * Put a pixel on the screen
 * @param x x coordinate of the pixel
 * @param y y coordinate of the pixel
 * @param color color of the pixel
 */
void fb_put_pixel(uint32_t x, uint32_t y, color_t color) {
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return;
    }
    if (x >= fb_get_width() || y >= fb_get_height()) return; // Out of bounds

    fb_mark_dirty(x, y, 1, 1);
    uint32_t offset = (y * kernel_fb_info.fb_pitch) + (x * (kernel_fb_info.fb_bpp / 8)) + bb_info.scroll_offset;
    if (offset >= bb_info.backbuffer_size) offset -= bb_info.backbuffer_size; // Wrap around for scrolling
    uint8_t* pixel_addr = bb_info.backbuffer + offset;
    
    if (kernel_fb_info.fb_bpp == 32) {
        pixel_addr[0] = color.b;
        pixel_addr[1] = color.g;
        pixel_addr[2] = color.r;
        pixel_addr[3] = color.a;
        return;
    } else if (kernel_fb_info.fb_bpp == 24) {
        pixel_addr[0] = color.b;
        pixel_addr[1] = color.g;
        pixel_addr[2] = color.r;
        return;
    } else {
        serial_printf("FB: Error: Unsupported bits per pixel: %d\n", kernel_fb_info.fb_bpp);
        return;
    }
}

/*
 * Get the color of a pixel on the screen
 * @param x x coordinate of the pixel
 * @param y y coordinate of the pixel
 * @return The color of the pixel at the given coordinates
 */
color_t fb_get_pixel(uint32_t x, uint32_t y) {
    color_t color = {0, 0, 0, 0};
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return color;
    }
    if (x >= fb_get_width() || y >= fb_get_height()) return color; // Out of bounds

    uint32_t offset = (y * kernel_fb_info.fb_pitch) + (x * (kernel_fb_info.fb_bpp / 8)) + bb_info.scroll_offset;
    if (offset >= bb_info.backbuffer_size) offset -= bb_info.backbuffer_size;
    uint8_t* pixel_addr = bb_info.backbuffer + offset;

    if (kernel_fb_info.fb_bpp == 32) {
        color.b = pixel_addr[0];
        color.g = pixel_addr[1];
        color.r = pixel_addr[2];
        color.a = pixel_addr[3];
        return color;
    } else if (kernel_fb_info.fb_bpp == 24) {
        color.b = pixel_addr[0];
        color.g = pixel_addr[1];
        color.r = pixel_addr[2];
        color.a = 255;
        return color;
    } else {
        serial_printf("FB: Error: Unsupported bits per pixel: %d\n", kernel_fb_info.fb_bpp);
        return color;
    }
}

/*
 * Draw a rectangle on the screen
 * @param x x coordinate of the top-left corner
 * @param y y coordinate of the top-left corner
 * @param width width of the rectangle
 * @param height height of the rectangle
 * @param color color of the rectangle
 */
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color) {
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return;
    }
    if (x >= fb_get_width() || y >= fb_get_height()) return;
    
    if (x + width > fb_get_width()) width = fb_get_width() - x;
    if (y + height > fb_get_height()) height = fb_get_height() - y;

    fb_mark_dirty(x, y, width, height);

    if (kernel_fb_info.fb_bpp == 32) {
        for (uint32_t i = 0; i < height; i++) {
            uint32_t row = y + i;
            uint32_t offset = (row * kernel_fb_info.fb_pitch) + (x * 4) + bb_info.scroll_offset;
            if (offset >= bb_info.backbuffer_size) offset -= bb_info.backbuffer_size;
            uint32_t* dest = (uint32_t*)(bb_info.backbuffer + offset);
            uint32_t val = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
            memset32(dest, val, width);
        }
    } else if (kernel_fb_info.fb_bpp == 24) {
        for (uint32_t i = 0; i < height; i++) {
            uint32_t row = y + i;
            uint32_t offset = (row * kernel_fb_info.fb_pitch) + (x * 3) + bb_info.scroll_offset;
            if (offset >= bb_info.backbuffer_size) offset -= bb_info.backbuffer_size;
            uint8_t* dest = bb_info.backbuffer + offset;
            for (uint32_t j = 0; j < width; j++) {
                dest[j * 3 + 0] = color.b;
                dest[j * 3 + 1] = color.g;
                dest[j * 3 + 2] = color.r;
            }
        }
    } else {
        serial_printf("FB: Error: Unsupported bits per pixel: %d\n", kernel_fb_info.fb_bpp);
        return;
    }
}

/*
 * Draw a character on the screen using the 8x8 font
 * @param x x coordinate of the top-left corner
 * @param y y coordinate of the top-left corner
 * @param c character to draw
 * @param fg_color foreground color
 * @param bg_color background color
 */
void fb_draw_char(uint32_t x, uint32_t y, char c, color_t fg_color, color_t bg_color) {
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return;
    }
    if (x >= fb_get_width() || y >= fb_get_height()) return;
    uint8_t* font_char = font8x8_basic[(uint8_t)c];
    if (!font_char) {
        serial_printf("FB: Error: Invalid character: %c\n", c);
        return;
    }

    fb_mark_dirty(x, y, FONT_WIDTH, FONT_HEIGHT);

    for (uint32_t j = 0; j < FONT_HEIGHT; j++) {
        for (uint32_t i = 0; i < FONT_WIDTH; i++) {
            color_t color = (font_char[j] & (1 << (0 + i))) ? fg_color : bg_color;
            fb_put_pixel(x + i, y + j, color);
        }
    }
}

/*
 * Scroll the framebuffer by a given number of lines
 * @param lines number of lines to scroll
 * @param color color to fill the new lines with
 */
void fb_scroll(uint32_t lines, color_t color) {
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return;
    }
    if (lines == 0) return;
    if (lines >= fb_get_height()) {
        fb_clear(color);
        return;
    }

    fb_mark_dirty(0, 0, fb_get_width(), fb_get_height());

    uint32_t bytes_to_scroll = lines * kernel_fb_info.fb_pitch;

    bb_info.scroll_offset += bytes_to_scroll;
    if (bb_info.scroll_offset >= bb_info.backbuffer_size) {
        bb_info.scroll_offset -= bb_info.backbuffer_size;
    }

    fb_draw_rect(0, fb_get_height() - lines, fb_get_width(), lines, color);
}

/*
 * Clear the framebuffer with a given color
 * @param color color to clear the screen with
 */
void fb_clear(color_t color) {
    fb_draw_rect(0, 0, fb_get_width(), fb_get_height(), color);
}

/*
 * Swap the backbuffer to the frontbuffer (physical framebuffer)
 * Only updates the dirty region for performance
 */
void fb_swap_buffers(void) {
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return;
    }
    if (dirty_x1 >= dirty_x2 || dirty_y1 >= dirty_y2) return; // No dirty region

    uint8_t* vram_base = (uint8_t*)kernel_fb_info.fb_addr;

    if (kernel_fb_info.fb_bpp == 32) {
        for (uint32_t y = dirty_y1; y < dirty_y2; y++) {
            uint32_t offset = (y * kernel_fb_info.fb_pitch) + (dirty_x1 * 4);
            uint32_t src_offset = offset + bb_info.scroll_offset;
            if (src_offset >= bb_info.backbuffer_size) src_offset -= bb_info.backbuffer_size;
            uint32_t* dest = (uint32_t*)(vram_base + offset);
            uint32_t* src = (uint32_t*)(bb_info.backbuffer + src_offset);
            memcpy32(dest, src, (dirty_x2 - dirty_x1));
        }
    } else if (kernel_fb_info.fb_bpp == 24) {
        for (uint32_t y = dirty_y1; y < dirty_y2; y++) {
            uint32_t offset = (y * kernel_fb_info.fb_pitch) + (dirty_x1 * 3);
            uint32_t src_offset = offset + bb_info.scroll_offset;
            if (src_offset >= bb_info.backbuffer_size) src_offset -= bb_info.backbuffer_size;
            uint8_t* dest = vram_base + offset;
            uint8_t* src = bb_info.backbuffer + src_offset;
            memcpy(dest, src, (dirty_x2 - dirty_x1) * 3);
        }
    } else {
        serial_printf("FB: Error: Unsupported bits per pixel: %d\n", kernel_fb_info.fb_bpp);
        return;
    }

    dirty_x1 = dirty_y1 = dirty_x2 = dirty_y2 = 0;
}