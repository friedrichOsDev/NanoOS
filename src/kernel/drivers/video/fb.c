#include <fb.h>
#include <font.h>
#include <heap.h>
#include <string.h>
#include <serial.h>

static fb_info_t fb_info;

void fb_init() {
    size_t buffer_size = screen_info->height * screen_info->bytes_per_line;
    
    serial_puts("fb_init: requesting buffer size: ");
    serial_put_int(buffer_size);
    serial_puts(" bytes.\n");

    fb_info.back_buffer = kmalloc(buffer_size);
    
    serial_puts("fb_init: back_buffer allocated at: 0x");
    serial_put_hex((uint32_t)fb_info.back_buffer);
    serial_puts("\n");

    if (!fb_info.back_buffer) {
        serial_puts("fb_init: failed to allocate back buffer\n");
        return;
    }
    fb_info.back_buffer_size = buffer_size;
    fb_clear(0x000000);
    fb_swap_buffers();
}

uint32_t fb_get_width() {
    return screen_info->width;
}

uint32_t fb_get_height() {
    return screen_info->height;
}

size_t fb_get_back_buffer_size() {
    return fb_info.back_buffer_size;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_info.back_buffer) return;

    if (x >= screen_info->width || y >= screen_info->height) {
        return;
    }

    uint32_t offset = (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel);

    if (screen_info->bytes_per_pixel == 4) {
        *((uint32_t*)(fb_info.back_buffer + offset)) = color;
    } else if (screen_info->bytes_per_pixel == 3) {
        uint8_t* pixel = fb_info.back_buffer + offset;
        pixel[0] = color & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = (color >> 16) & 0xFF;
    }
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!fb_info.back_buffer) return;

    for (uint32_t j = 0; j < height; j++) {
        uint32_t row = y + j;
        if (row >= screen_info->height) continue;

        uint32_t offset = (row * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel);
        
        uint32_t draw_width = width;
        if (x + draw_width > screen_info->width) {
            draw_width = screen_info->width - x;
        }

        if (screen_info->bytes_per_pixel == 4) {
            // optimized for 32bpp
            uint32_t* dest = (uint32_t*)(fb_info.back_buffer + offset);
            for (uint32_t i = 0; i < draw_width; i++) {
                dest[i] = color;
            }
        } else {
            // fallback for other bpp
            for (uint32_t i = 0; i < draw_width; i++) {
                fb_put_pixel(x + i, row, color);
            }
        }
    }
}

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
    uint8_t* font_char = font8x8_basic[(uint8_t)c];

    for (int i = 0; i < FONT_HEIGHT; i++) {
        for (int j = 0; j < FONT_WIDTH; j++) {
            uint32_t color = ((font_char[i] >> j) & 0x01) ? fg_color : bg_color;
            fb_put_pixel(x + j, y + i, color);
        }
    }
}

void fb_scroll(uint32_t lines, uint32_t color) {
    if (!fb_info.back_buffer) return;

    if (lines == 0) return;
    if (lines >= screen_info->height) {
        fb_clear(color);
        return;
    }

    uint32_t line_bytes = screen_info->bytes_per_line;
    uint32_t lines_in_bytes = lines * line_bytes;

    // move existing content up
    uint32_t bytes_to_move = (screen_info->height - lines) * line_bytes;
    memcpy(fb_info.back_buffer, fb_info.back_buffer + lines_in_bytes, bytes_to_move);

    // clear the new lines at the bottom
    uint32_t offset = bytes_to_move;
    if (screen_info->bytes_per_pixel == 4) {
        uint32_t* dest = (uint32_t*)(fb_info.back_buffer + offset);
        size_t num_pixels = (fb_info.back_buffer_size - offset) / 4;
        for (size_t i = 0; i < num_pixels; i++) {
            dest[i] = color;
        }
    } else {
        // fallback for other bpp
        for (uint32_t y = screen_info->height - lines; y < screen_info->height; y++) {
            for (uint32_t x = 0; x < screen_info->width; x++) {
                fb_put_pixel(x, y, color);
            }
        }
    }
}

void fb_clear(uint32_t color) {
    if (!fb_info.back_buffer) return;

    if (screen_info->bytes_per_pixel == 4) {
        // optimized for 32bpp
        uint32_t* dest = (uint32_t*)fb_info.back_buffer;
        size_t num_pixels = fb_info.back_buffer_size / 4;
        for (size_t i = 0; i < num_pixels; i++) {
            dest[i] = color;
        }
    } else {
        // fallback for other bpp
        for (uint32_t y = 0; y < screen_info->height; y++) {
            for (uint32_t x = 0; x < screen_info->width; x++) {
                fb_put_pixel(x, y, color);
            }
        }
    }
}

void fb_swap_buffers() {
    if (!fb_info.back_buffer) return;
    memcpy((void*)screen_info->physical_buffer, fb_info.back_buffer, fb_info.back_buffer_size);
}