#include <fb.h>
#include <font.h>
#include <heap.h>
#include <string.h>
#include <serial.h>
#include <cpu.h>

static fb_info_t fb_info;
static uint32_t dirty_x1;
static uint32_t dirty_y1;
static uint32_t dirty_x2;
static uint32_t dirty_y2;

// optimized 32-bit memory fill
static inline void memset32(void* dest, uint32_t val, size_t count) {
    __asm__ __volatile__ ("cld; rep stosl" : "+D"(dest), "+c"(count) : "a"(val) : "memory");
}

// optimized 32-bit memory copy
static inline void memcpy32(void* dest, const void* src, size_t count) {
    __asm__ __volatile__ ("cld; rep movsl" : "+D"(dest), "+S"(src), "+c"(count) : : "memory");
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
    uint32_t scroll_offset = 0;
    
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
    fb_info.scroll_offset = scroll_offset;

    uint32_t pot_size = 1;
    while (pot_size < buffer_size) {
        pot_size <<= 1;
    }

    cpu_enable_write_combining(screen_info->physical_buffer, pot_size);

    // reset dirty rect
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
    if (screen_info->bytes_per_pixel != 4) return;
    if (x >= screen_info->width || y >= screen_info->height) return;

    fb_mark_dirty(x, y, 1, 1);

    uint32_t offset = (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel) + fb_info.scroll_offset;
    if (offset >= fb_info.back_buffer_size) offset -= fb_info.back_buffer_size;

    *((uint32_t*)(fb_info.back_buffer + offset)) = color;
}

uint32_t fb_get_pixel(uint32_t x, uint32_t y) {
    if (!fb_info.back_buffer) return 0;
    if (screen_info->bytes_per_pixel != 4) return 0;
    if (x >= screen_info->width || y >= screen_info->height) return 0;

    uint32_t offset = (y * screen_info->bytes_per_line) + (x * screen_info->bytes_per_pixel) + fb_info.scroll_offset;
    if (offset >= fb_info.back_buffer_size) offset -= fb_info.back_buffer_size;

    return *((uint32_t*)(fb_info.back_buffer + offset));
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;
    if (x >= screen_info->width || y >= screen_info->height) return;

    // clip width and height to screen boundaries
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

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;
    uint8_t* font_char = font8x8_basic[(uint8_t)c];
    if (!font_char) return;

    fb_mark_dirty(x, y, FONT_WIDTH, FONT_HEIGHT);

    // calc starting offset
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

    // clear the newly exposed area
    if (old_offset + bytes_to_scroll > fb_info.back_buffer_size) {
        uint32_t part1_len = fb_info.back_buffer_size - old_offset;
        uint32_t part2_len = bytes_to_scroll - part1_len;
        memset32(fb_info.back_buffer + old_offset, color, part1_len / 4);
        memset32(fb_info.back_buffer, color, part2_len / 4);
    } else {
        memset32(fb_info.back_buffer + old_offset, color, bytes_to_scroll / 4);
    }
}

void fb_clear(uint32_t color) {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;

    uint32_t* dest = (uint32_t*)fb_info.back_buffer;
    size_t num_pixels = fb_info.back_buffer_size / 4;
    memset32(dest, color, num_pixels);
    fb_mark_dirty(0, 0, screen_info->width, screen_info->height);
}

void fb_swap_buffers() {
    if (!fb_info.back_buffer) return;
    if (screen_info->bytes_per_pixel != 4) return;

    if (dirty_x2 == 0 && dirty_y2 == 0) return; // nothing to update

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

    // reset dirty rect
    dirty_x1 = dirty_y1 = dirty_x2 = dirty_y2 = 0;

    // clear CPU cache 
    __asm__ __volatile__ ("mfence" ::: "memory");
}