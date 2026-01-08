#include <fb.h>
#include <font.h>

void fb_init() {
    // initialize framebuffer
    fb_clear(0x000000);
}

uint32_t fb_get_width() {
    // return framebuffer width
    return screen_info->width;
}

uint32_t fb_get_height() {
    // return framebuffer height
    return screen_info->height;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    // put a pixel at (x, y) with the specified color
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
    // draw a rectangle at (x, y) with specified width, height and color
    for (uint32_t i = 0; i < width; i++) {
        for (uint32_t j = 0; j < height; j++) {
            fb_put_pixel(x + i, y + j, color);
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
    uint32_t fb = screen_info->physical_buffer;
    uint32_t bytes_to_scroll = lines * screen_info->bytes_per_line;
    uint32_t total_bytes = screen_info->height * screen_info->bytes_per_line;

    // move existing content up
    for (uint32_t i = 0; i < total_bytes - bytes_to_scroll; i++) {
        *((uint8_t*)(fb + i)) = *((uint8_t*)(fb + i + bytes_to_scroll));
    }

    // clear the new lines at the bottom using put pixel
    for (uint32_t y = screen_info->height - lines; y < screen_info->height; y++) {
        for (uint32_t x = 0; x < screen_info->width; x++) {
            fb_put_pixel(x, y, color);
        }
    }
    
}

void fb_clear(uint32_t color) {
    // clear the framebuffer with the specified color
    for (uint32_t x = 0; x < screen_info->width; x++) {
        for (uint32_t y = 0; y < screen_info->height; y++) {
            fb_put_pixel(x, y, color);
        }
    }
}