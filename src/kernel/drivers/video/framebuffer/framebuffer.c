/**
 * @file framebuffer.c
 * @brief Framebuffer driver implementation
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/memdef.h>
#include <core/init.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <drivers/video/framebuffer/framebuffer.h>
#include <lib/string.h>
#include <stdbool.h>
#include <stdint.h>

color_t black = {0, 0, 0, 255};
color_t gray = {128, 128, 128, 255};
color_t white = {255, 255, 255, 255};
color_t red = {0, 0, 255, 255};
color_t green = {0, 255, 0, 255};
color_t blue = {255, 0, 0, 255};

// Triple-Buffering Pointers
static uint8_t *fb_write_buf = NULL;   // Buffer für Zeichen-Operationen
static uint8_t *fb_pending_buf = NULL; // Bereit zum Kopieren
static uint8_t *fb_work_buf = NULL;    // Tausch-Puffer
static size_t fb_buffer_size = 0;
static volatile bool fb_has_new_frame = false;

static inline int64_t abs64(int64_t n) { return n < 0 ? -n : n; }

uint64_t fb_get_width(void) { return kernel_fb_info.fb_width; }
uint64_t fb_get_height(void) { return kernel_fb_info.fb_height; }
size_t fb_get_backbuffer_size(void) { return fb_buffer_size; }

void fb_clear(color_t color) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    uint64_t width = kernel_fb_info.fb_width;
    uint64_t height = kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    for (uint64_t y = 0; y < height; y++) {
        memset32(fb_write_buf + (y * pitch), color_val, width);
    }
}

void fb_draw_pixel(uint64_t x, uint64_t y, color_t color) {
    if (x >= kernel_fb_info.fb_width || y >= kernel_fb_info.fb_height)
        return;

    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    uint64_t offset = (y * kernel_fb_info.fb_pitch) + (x * 4);
    *(uint32_t *)(fb_write_buf + offset) = color_val;
}

void fb_draw_rectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                       color_t color, bool filled, uint64_t border_size) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    uint64_t swidth = kernel_fb_info.fb_width;
    uint64_t sheight = kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    if (x >= swidth || y >= sheight)
        return;
    if (x + width > swidth)
        width = swidth - x;
    if (y + height > sheight)
        height = sheight - y;

    if (filled) {
        for (uint64_t i = 0; i < height; i++) {
            memset32(fb_write_buf + ((y + i) * pitch) + (x * 4), color_val,
                     width);
        }
    } else {
        if (border_size > width)
            border_size = width;
        if (border_size > height)
            border_size = height;

        for (uint64_t i = 0; i < border_size; i++) {
            memset32(fb_write_buf + ((y + i) * pitch) + (x * 4), color_val,
                     width);
            memset32(fb_write_buf + ((y + height - border_size + i) * pitch) +
                         (x * 4),
                     color_val, width);
        }
        for (uint64_t i = border_size; i < height - border_size; i++) {
            memset32(fb_write_buf + ((y + i) * pitch) + (x * 4), color_val,
                     border_size);
            memset32(fb_write_buf + ((y + i) * pitch) +
                         ((x + width - border_size) * 4),
                     color_val, border_size);
        }
    }
}

void fb_draw_circle(uint64_t xc, uint64_t yc, uint64_t radius, color_t color,
                    bool filled, uint64_t border_size) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    uint64_t swidth = kernel_fb_info.fb_width;
    uint64_t sheight = kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    if (filled) {
        int64_t x = 0, y = radius;
        int64_t d = 3 - (2 * (int64_t)radius);

        while (x <= y) {
#define DRAW_H_LINE(xs, xe, yp)                                                \
    if ((yp) >= 0 && (yp) < (int64_t)sheight) {                                \
        int64_t _s = (xs) < 0 ? 0 : (xs);                                      \
        int64_t _e = (xe) >= (int64_t)swidth ? (int64_t)swidth - 1 : (xe);     \
        if (_s <= _e)                                                          \
            memset32(fb_write_buf + ((yp) * pitch) + (_s * 4), color_val,      \
                     _e - _s + 1);                                             \
    }

            DRAW_H_LINE((int64_t)xc - x, (int64_t)xc + x, (int64_t)yc + y);
            DRAW_H_LINE((int64_t)xc - x, (int64_t)xc + x, (int64_t)yc - y);
            DRAW_H_LINE((int64_t)xc - y, (int64_t)xc + y, (int64_t)yc + x);
            DRAW_H_LINE((int64_t)xc - y, (int64_t)xc + y, (int64_t)yc - x);
#undef DRAW_H_LINE

            if (d < 0)
                d += (4 * x) + 6;
            else {
                d += (4 * (x - y)) + 10;
                y--;
            }
            x++;
        }
    } else {
        for (uint64_t r = radius - border_size + 1; r <= radius; r++) {
            int64_t x = 0, y = r;
            int64_t d = 3 - (2 * (int64_t)r);

            while (x <= y) {
                fb_draw_pixel(xc + x, yc + y, color);
                fb_draw_pixel(xc - x, yc + y, color);
                fb_draw_pixel(xc + x, yc - y, color);
                fb_draw_pixel(xc - x, yc - y, color);
                fb_draw_pixel(xc + y, yc + x, color);
                fb_draw_pixel(xc - y, yc + x, color);
                fb_draw_pixel(xc + y, yc - x, color);
                fb_draw_pixel(xc - y, yc - x, color);

                if (d < 0)
                    d += (4 * x) + 6;
                else {
                    d += (4 * (x - y)) + 10;
                    y--;
                }
                x++;
            }
        }
    }
}

void fb_draw_triangle(uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2,
                      uint64_t x3, uint64_t y3, color_t color, bool filled,
                      uint64_t border_size) {
    if (!filled) {
        fb_draw_line(x1, y1, x2, y2, color, border_size);
        fb_draw_line(x2, y2, x3, y3, color, border_size);
        fb_draw_line(x3, y3, x1, y1, color, border_size);
        return;
    }

    if (y1 > y2) {
        uint64_t t = y1;
        y1 = y2;
        y2 = t;
        t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y2 > y3) {
        uint64_t t = y2;
        y2 = y3;
        y3 = t;
        t = x2;
        x2 = x3;
        x3 = t;
    }
    if (y1 > y2) {
        uint64_t t = y1;
        y1 = y2;
        y2 = t;
        t = x1;
        x1 = x2;
        x2 = t;
    }

    int64_t total_height = y3 - y1;
    if (total_height == 0)
        return;

    for (int64_t i = 0; i < total_height; i++) {
        bool second_half = i > (int64_t)(y2 - y1) || y2 == y1;
        int64_t segment_height =
            second_half ? (int64_t)(y3 - y2) : (int64_t)(y2 - y1);
        if (segment_height == 0)
            continue;

        int64_t ax =
            (int64_t)x1 + ((int64_t)x3 - (int64_t)x1) * i / total_height;
        int64_t bx = second_half ? (int64_t)x2 + ((int64_t)x3 - (int64_t)x2) *
                                                     (i - (int64_t)(y2 - y1)) /
                                                     segment_height
                                 : (int64_t)x1 + ((int64_t)x2 - (int64_t)x1) *
                                                     i / segment_height;

        if (ax > bx) {
            int64_t t = ax;
            ax = bx;
            bx = t;
        }
        fb_draw_line(ax, y1 + i, bx, y1 + i, color, 1);
    }
}

void fb_draw_line(int64_t x1, int64_t y1, int64_t x2, int64_t y2, color_t color,
                  uint64_t thickness) {
    if (thickness == 0)
        return;

    if (thickness == 1) {
        int64_t dx = abs64(x2 - x1);
        int64_t dy = -abs64(y2 - y1);
        int64_t sx = x1 < x2 ? 1 : -1;
        int64_t sy = y1 < y2 ? 1 : -1;
        int64_t err = dx + dy;

        while (1) {
            if (x1 >= 0 && x1 < (int64_t)kernel_fb_info.fb_width && y1 >= 0 &&
                y1 < (int64_t)kernel_fb_info.fb_height) {
                fb_draw_pixel((uint64_t)x1, (uint64_t)y1, color);
            }

            if (x1 == x2 && y1 == y2)
                break;
            int64_t e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x1 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y1 += sy;
            }
        }
        return;
    }

    int64_t dx = abs64(x2 - x1);
    int64_t dy = abs64(y2 - y1);
    int64_t half_thick = thickness / 2;

    if (dx >= dy) {
        for (int64_t offset = -half_thick;
             offset < (int64_t)thickness - half_thick; offset++) {
            fb_draw_line(x1, y1 + offset, x2, y2 + offset, color, 1);
        }
    } else {
        for (int64_t offset = -half_thick;
             offset < (int64_t)thickness - half_thick; offset++) {
            fb_draw_line(x1 + offset, y1, x2 + offset, y2, color, 1);
        }
    }
}

void fb_swap_buffers(void) {
    uint8_t *completed_frame = fb_write_buf;
    fb_write_buf = fb_work_buf;

    __atomic_store_n(&fb_pending_buf, completed_frame, __ATOMIC_RELEASE);
    __atomic_store_n(&fb_has_new_frame, true, __ATOMIC_RELEASE);
}

void frame_swap_thread(void *arg) {
    (void)arg;
    while (1) {
        if (__atomic_load_n(&fb_has_new_frame, __ATOMIC_ACQUIRE)) {
            uint8_t *to_present =
                __atomic_load_n(&fb_pending_buf, __ATOMIC_ACQUIRE);
            if (to_present && kernel_fb_info.fb_addr) {
                memcpy((void *)kernel_fb_info.fb_addr, to_present,
                       fb_buffer_size);
                fb_work_buf = to_present;
            }
            __atomic_store_n(&fb_has_new_frame, false, __ATOMIC_RELEASE);
        }
        thread_sleep_ms(16);
    }
}

void framebuffer_init_thread(void *arg) {
    (void)arg;
    fb_buffer_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;

    serial_printf(COM1, "FB: Initializing Triple Buffering (size: %d bytes)\n",
                  fb_buffer_size);

    fb_write_buf = (uint8_t *)kzalloc(fb_buffer_size);
    fb_pending_buf = (uint8_t *)kzalloc(fb_buffer_size);
    fb_work_buf = (uint8_t *)kzalloc(fb_buffer_size);

    if (!fb_write_buf || !fb_pending_buf || !fb_work_buf) {
        serial_printf(COM1, "FB: Failed to allocate framebuffers!\n");
        return;
    }

    thread_create(NULL, frame_swap_thread, NULL, "frame_swap_thread");

    scheduler_thread_exit();
}