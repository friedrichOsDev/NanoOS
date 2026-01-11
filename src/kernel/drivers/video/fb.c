#include <fb.h>
#include <font.h>
#include <heap.h>
#include <string.h>
#include <serial.h>

static fb_info_t fb_info;
static uint32_t dirty_x1;
static uint32_t dirty_y1;
static uint32_t dirty_x2;
static uint32_t dirty_y2;

// optimized 32-bit memory fill
static inline void memset32(void* dest, uint32_t val, size_t count) {
    asm volatile ("cld; rep stosl" : "+D"(dest), "+c"(count) : "a"(val) : "memory");
}

// optimized 32-bit memory copy
static inline void memcpy32(void* dest, const void* src, size_t count) {
    asm volatile ("cld; rep movsl" : "+D"(dest), "+S"(src), "+c"(count) : : "memory");
}

static void fb_mark_dirty(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!fb_info.back_buffer) return;
    if (x >= screen_info->width || y >= screen_info->height) return;

    uint32_t x_end = x + w;
    uint32_t y_end = y + h;

    if (x_end > screen_info->width) x_end = screen_info->width;
    if (y_end > screen_info->height) y_end = screen_info->height;

    if (dirty_x2 == 0 && dirty_y2 == 0) {
        // initialize dirty rect
        dirty_x1 = x;
        dirty_y1 = y;
        dirty_x2 = x_end;
        dirty_y2 = y_end;
    } else {
        // expand dirty rect
        if (x < dirty_x1) dirty_x1 = x;
        if (y < dirty_y1) dirty_y1 = y;
        if (x_end > dirty_x2) dirty_x2 = x_end;
        if (y_end > dirty_y2) dirty_y2 = y_end;
    }
}

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
    
    // reset dirty rects
    dirty_x1 = dirty_y1 = dirty_x2 = dirty_y2 = 0;
    
    fb_clear(0x000000);
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

    fb_mark_dirty(x, y, 1, 1);

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

    fb_mark_dirty(x, y, width, height);

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
            memset32(dest, color, draw_width);
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

    if (!font_char) return;
    if (x >= screen_info->width || y >= screen_info->height) return;
    if (x + FONT_WIDTH > screen_info->width || y + FONT_HEIGHT > screen_info->height) return;
    if (fg_color == bg_color) {
        fb_draw_rect(x, y, FONT_WIDTH, FONT_HEIGHT, bg_color);
        return;
    }
    
    fb_mark_dirty(x, y, FONT_WIDTH, FONT_HEIGHT);

    if (screen_info->bytes_per_pixel == 4) {
        // optimized for 32bpp
        uint32_t* dest = (uint32_t*)(fb_info.back_buffer + (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel));
        uint32_t line_offset = screen_info->bytes_per_line / 4;

        for (int i = 0; i < FONT_WIDTH; i++) {
            for (int j = 0; j < FONT_HEIGHT; j++) {
                dest[j] = ((font_char[i] >> j) & 0x01) ? fg_color : bg_color;
            }
            dest += line_offset;
        }
    } else {
        // fallback for other bpp
        for (int i = 0; i < FONT_WIDTH; i++) {
            for (int j = 0; j < FONT_HEIGHT; j++) {
                uint32_t pixel_color = ((font_char[i] >> j) & 0x01) ? fg_color : bg_color;
                fb_put_pixel(x + i, y + j, pixel_color);
            }
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

    // mark the whole screen as dirty
    fb_mark_dirty(0, 0, screen_info->width, screen_info->height);

    uint32_t line_bytes = screen_info->bytes_per_line;
    uint32_t lines_in_bytes = lines * line_bytes;

    // move existing content up
    uint32_t bytes_to_move = (screen_info->height - lines) * line_bytes;
    
    // use optimized copy if aligned
    if (lines_in_bytes % 4 == 0 && bytes_to_move % 4 == 0) {
        memcpy32(fb_info.back_buffer, fb_info.back_buffer + lines_in_bytes, bytes_to_move / 4);
    } else {
        memcpy(fb_info.back_buffer, fb_info.back_buffer + lines_in_bytes, bytes_to_move);
    }

    // clear the new lines at the bottom
    uint32_t offset = bytes_to_move;
    if (screen_info->bytes_per_pixel == 4) {
        uint32_t* dest = (uint32_t*)(fb_info.back_buffer + offset);
        size_t num_pixels = (fb_info.back_buffer_size - offset) / 4;
        memset32(dest, color, num_pixels);
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

    fb_mark_dirty(0, 0, screen_info->width, screen_info->height);

    if (screen_info->bytes_per_pixel == 4) {
        // optimized for 32bpp
        uint32_t* dest = (uint32_t*)fb_info.back_buffer;
        size_t num_pixels = fb_info.back_buffer_size / 4;
        memset32(dest, color, num_pixels);
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

    // if nothing is dirty, do nothing
    if (dirty_x2 <= dirty_x1 || dirty_y2 <= dirty_y1) {
        return;
    }

    uint32_t bpp = screen_info->bytes_per_pixel;
    uint32_t pitch = screen_info->bytes_per_line;
    uint32_t row_len = (dirty_x2 - dirty_x1) * bpp;

    // copy only the dirty lines
    for (uint32_t y = dirty_y1; y < dirty_y2; y++) {
        uint32_t offset = (y * pitch) + (dirty_x1 * bpp);
        void* dest = (void*)(screen_info->physical_buffer + offset);
        void* src = fb_info.back_buffer + offset;

        if (bpp == 4) {
            memcpy32(dest, src, row_len / 4);
        } else {
            memcpy(dest, src, row_len);
        }
    }

    // reset dirty rect
    dirty_x1 = 0;
    dirty_y1 = 0;
    dirty_x2 = 0;
    dirty_y2 = 0;
}