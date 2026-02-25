/**
 * @file fb.c
 * @author friedrichOsDev
 */

#include <fb.h>
#include <kernel.h>
#include <serial.h>
#include <font.h>
#include <string.h>
#include <heap.h>
#include <timer.h>

static backbuffer_info_t bb_info;
static uint32_t dirty_x1;
static uint32_t dirty_y1;
static uint32_t dirty_x2;
static uint32_t dirty_y2;
static bool update_flag = false;
color_t black = {255, 0, 0, 0};
color_t white = {255, 255, 255, 255};
color_t red = {255, 255, 0, 0};
color_t green = {255, 0, 255, 0};
color_t blue = {255, 0, 0, 255};

/**
 * @brief Fast 32-bit memory set using x86 assembly.
 * @param dest Destination address.
 * @param value 32-bit value to set.
 * @param count Number of 32-bit words to set.
 */
static inline void memset32(void* dest, uint32_t value, size_t count) {
    __asm__ __volatile__(
        "rep stosl"
        : "+D" (dest), "+c" (count)
        : "a" (value)
        : "memory"
    );
}

/**
 * @brief Fast 32-bit memory copy using x86 assembly.
 * @param dest Destination address.
 * @param src Source address.
 * @param count Number of 32-bit words to copy.
 */
static inline void memcpy32(void* dest, const void* src, size_t count) {
    __asm__ __volatile__(
        "rep movsl"
        : "+D" (dest), "+S" (src), "+c" (count)
        :
        : "memory"
    );
}

/**
 * @brief Expands the current dirty region to include the specified rectangle.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param w Width.
 * @param h Height.
 */
static void fb_mark_dirty(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (x < dirty_x1) dirty_x1 = x;
    if (y < dirty_y1) dirty_y1 = y;
    if (x + w > dirty_x2) dirty_x2 = x + w;
    if (y + h > dirty_y2) dirty_y2 = y + h;
    if (dirty_x2 > fb_get_width()) dirty_x2 = fb_get_width();
    if (dirty_y2 > fb_get_height()) dirty_y2 = fb_get_height();
}

/**
 * @brief Timer callback to signal that a buffer swap is requested.
 */
static void fb_update_event(void) {
    update_flag = true;
}

/**
 * @brief Checks if an update is pending and performs a buffer swap if necessary.
 * Should be called in the main kernel loop.
 */
void fb_update(void) {
    if (update_flag) {
        fb_swap_buffers();
        update_flag = false;
    }
}

/**
 * @brief Initializes the framebuffer subsystem.
 * Allocates the backbuffer and sets up the periodic update timer.
 */
void fb_init(void) {
    serial_printf("FB: start\n");
    size_t buffer_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
    uint32_t scroll_offset = 0;
    serial_printf("FB: Initializing framebuffer: %dx%d, %d bpp, pitch: %d, buffer size: %d bytes\n", kernel_fb_info.fb_width, kernel_fb_info.fb_height, kernel_fb_info.fb_bpp, kernel_fb_info.fb_pitch, buffer_size);
    bb_info.backbuffer = (uint8_t*)kzalloc(buffer_size);
    serial_printf("FB: Backbuffer Virt: %x, Phys: %x\n", bb_info.backbuffer, vmm_virtual_to_physical(vmm_get_page_directory(), (virt_addr_t)bb_info.backbuffer));
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Failed to allocate backbuffer\n");
        return;
    }
    bb_info.backbuffer_size = buffer_size;
    bb_info.scroll_offset = scroll_offset;
    
    dirty_x1 = dirty_y1 = 0xFFFFFFFF;
    dirty_x2 = dirty_y2 = 0;

    serial_printf("FB: clear screen\n");
    fb_clear(black);
    event_t update_event = {
        .event_id = 0,
        .handler = fb_update_event,
        .interval = 1,
        .target_tick = timer_get_ticks() + 1,
        .repeat = true,
        .active = true
    };

    uint32_t update_event_id = timer_add_event(update_event);
    (void)update_event_id;

    serial_printf("FB: done\n");
}

/**
 * @brief Returns the screen width in pixels.
 */
uint32_t fb_get_width(void) {
    return kernel_fb_info.fb_width;
}

/**
 * @brief Returns the screen height in pixels.
 */
uint32_t fb_get_height(void) {
    return kernel_fb_info.fb_height;
}

/**
 * @brief Returns the size of the backbuffer in bytes.
 */
size_t fb_get_backbuffer_size(void) {
    return bb_info.backbuffer_size;
}

/**
 * @brief Draws a single pixel to the backbuffer.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param color Color to draw. Supports alpha blending on 32bpp.
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
        if (color.a < 255) {
            uint32_t inv_a = 255 - color.a;
            pixel_addr[0] = (uint8_t)((color.b * color.a + pixel_addr[0] * inv_a) / 255);
            pixel_addr[1] = (uint8_t)((color.g * color.a + pixel_addr[1] * inv_a) / 255);
            pixel_addr[2] = (uint8_t)((color.r * color.a + pixel_addr[2] * inv_a) / 255);
            pixel_addr[3] = color.a;
        } else {
            pixel_addr[0] = color.b;
            pixel_addr[1] = color.g;
            pixel_addr[2] = color.r;
            pixel_addr[3] = color.a;
        }
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

/**
 * @brief Retrieves the color of a pixel from the backbuffer.
 * @param x X coordinate.
 * @param y Y coordinate.
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

/**
 * @brief Draws a filled rectangle.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param width Width.
 * @param height Height.
 * @param color Color of the rectangle.
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
        if (color.a < 255) {
            uint32_t inv_a = 255 - color.a;
            for (uint32_t i = 0; i < height; i++) {
                uint32_t row = y + i;
                uint32_t offset = (row * kernel_fb_info.fb_pitch) + (x * 4) + bb_info.scroll_offset;
                if (offset >= bb_info.backbuffer_size) offset -= bb_info.backbuffer_size;
                uint32_t* dest = (uint32_t*)(bb_info.backbuffer + offset);
                for (uint32_t j = 0; j < width; j++) {
                    uint32_t existing_pixel = dest[j];
                    uint8_t existing_b = existing_pixel & 0xFF;
                    uint8_t existing_g = (existing_pixel >> 8) & 0xFF;
                    uint8_t existing_r = (existing_pixel >> 16) & 0xFF;
                    dest[j] = (
                        (color.a << 24) | 
                        ((color.r * color.a + existing_r * inv_a) / 255 << 16) | 
                        ((color.g * color.a + existing_g * inv_a) / 255 << 8) | 
                        ((color.b * color.a + existing_b * inv_a) / 255)
                    );
                }
            }
        } else {
            for (uint32_t i = 0; i < height; i++) {
                uint32_t row = y + i;
                uint32_t offset = (row * kernel_fb_info.fb_pitch) + (x * 4) + bb_info.scroll_offset;
                if (offset >= bb_info.backbuffer_size) offset -= bb_info.backbuffer_size;
                uint32_t* dest = (uint32_t*)(bb_info.backbuffer + offset);
                uint32_t val = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
                memset32(dest, val, width);
            }
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

/**
 * @brief Draws a character using the built-in 8x8 font.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param c Character to draw.
 * @param fg_color Foreground color.
 * @param bg_color Background color.
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

/**
 * @brief Scrolls the entire screen upwards.
 * @param lines Number of lines to scroll.
 * @param color Color to fill the newly exposed area at the bottom.
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

/**
 * @brief Scrolls a specific rectangular region upwards.
 * @param x X coordinate of the region.
 * @param y Y coordinate of the region.
 * @param width Width of the region.
 * @param height Height of the region.
 * @param lines Number of lines to scroll.
 * @param color Color to fill the newly exposed area.
 */
void fb_scroll_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t lines, color_t color) {
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return;
    }
    if (lines == 0) return;
    if (lines >= height) {
        fb_draw_rect(x, y, width, height, color);
        return;
    }

    fb_mark_dirty(x, y, width, height);

    for (uint32_t i = 0; i < height - lines; i++) {
        uint32_t src_offset = ((y + lines + i) * kernel_fb_info.fb_pitch) + (x * (kernel_fb_info.fb_bpp / 8)) + bb_info.scroll_offset;
        if (src_offset >= bb_info.backbuffer_size) src_offset -= bb_info.backbuffer_size;
        uint32_t dest_offset = ((y + i) * kernel_fb_info.fb_pitch) + (x * (kernel_fb_info.fb_bpp / 8)) + bb_info.scroll_offset;
        if (dest_offset >= bb_info.backbuffer_size) dest_offset -= bb_info.backbuffer_size;

        if (kernel_fb_info.fb_bpp == 32) {
            uint32_t* src = (uint32_t*)(bb_info.backbuffer + src_offset);
            uint32_t* dest = (uint32_t*)(bb_info.backbuffer + dest_offset);
            memcpy32(dest, src, width);
        } else if (kernel_fb_info.fb_bpp == 24) {
            uint8_t* src = bb_info.backbuffer + src_offset;
            uint8_t* dest = bb_info.backbuffer + dest_offset;
            memcpy(dest, src, width * 3);
        } else {
            serial_printf("FB: Error: Unsupported bits per pixel: %d\n", kernel_fb_info.fb_bpp);
            return;
        }
    }

    fb_draw_rect(x, y + height - lines, width, lines, color);
}

/**
 * @brief Clears the entire screen with a specific color.
 */
void fb_clear(color_t color) {
    fb_draw_rect(0, 0, fb_get_width(), fb_get_height(), color);
}

/**
 * @brief Swaps the backbuffer to the frontbuffer (VRAM).
 * Only copies the region marked as dirty since the last swap.
 * Disables interrupts during dirty-rect reset to ensure atomicity.
 */
void fb_swap_buffers(void) {
    if (!bb_info.backbuffer) {
        serial_printf("FB: Error: Backbuffer not initialized\n");
        return;
    }

    __asm__ __volatile__("cli");

    if (dirty_x1 >= dirty_x2 || dirty_y1 >= dirty_y2) {
        __asm__ __volatile__("sti");
        return; // No dirty region
    }

    uint32_t x1 = dirty_x1;
    uint32_t y1 = dirty_y1;
    uint32_t x2 = dirty_x2;
    uint32_t y2 = dirty_y2;

    dirty_x1 = dirty_y1 = 0xFFFFFFFF;
    dirty_x2 = dirty_y2 = 0;

    __asm__ __volatile__("sti");

    uint8_t* vram_base = (uint8_t*)kernel_fb_info.fb_addr;

    if (kernel_fb_info.fb_bpp == 32) {
        for (uint32_t y = y1; y < y2; y++) {
            uint32_t offset = (y * kernel_fb_info.fb_pitch) + (x1 * 4);
            uint32_t src_offset = offset + bb_info.scroll_offset;
            if (src_offset >= bb_info.backbuffer_size) src_offset -= bb_info.backbuffer_size;
            uint32_t* dest = (uint32_t*)(vram_base + offset);
            uint32_t* src = (uint32_t*)(bb_info.backbuffer + src_offset);
            memcpy32(dest, src, (x2 - x1));
        }
    } else if (kernel_fb_info.fb_bpp == 24) {
        for (uint32_t y = y1; y < y2; y++) {
            uint32_t offset = (y * kernel_fb_info.fb_pitch) + (x1 * 3);
            uint32_t src_offset = offset + bb_info.scroll_offset;
            if (src_offset >= bb_info.backbuffer_size) src_offset -= bb_info.backbuffer_size;
            uint8_t* dest = vram_base + offset;
            uint8_t* src = bb_info.backbuffer + src_offset;
            memcpy(dest, src, (x2 - x1) * 3);
        }
    } else {
        serial_printf("FB: Error: Unsupported bits per pixel: %d\n", kernel_fb_info.fb_bpp);
        return;
    }
}