#include "../../include/fb.h"

void fb_init() {
    // Initialize framebuffer
    fb_clear(0x000000);
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    // Put a pixel at (x, y) with the specified color
    if (x >= screen_info->width || y >= screen_info->height) {
        return;
    }

    uint32_t fb = screen_info->physical_buffer;
    uint32_t offset = (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel);

    if (screen_info->bytes_per_pixel == 4) {
        *((uint32_t*)(fb + offset)) = color;
    } else if (screen_info->bytes_per_pixel == 3) {
        uint8_t *pixel = (uint8_t *)(fb + offset);
        pixel[0] = color & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = (color >> 16) & 0xFF;
    }
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    // Draw a rectangle at (x, y) with specified width, height and color
    for (uint32_t i = 0; i < width; i++) {
        for (uint32_t j = 0; j < height; j++) {
            fb_put_pixel(x + i, y + j, color);
        }
    }
}

void fb_clear(uint32_t color) {
    // Clear the framebuffer with the specified color
    for (uint32_t x = 0; x < screen_info->width; x++) {
        for (uint32_t y = 0; y < screen_info->height; y++) {
            fb_put_pixel(x, y, color);
        }
    }
}