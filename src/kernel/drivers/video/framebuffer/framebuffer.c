/**
 * @file framebuffer.c
 * @brief Simple Framebuffer Driver
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/memdef.h>
#include <core/init.h>
#include <core/scheduler.h>
#include <core/sync.h>
#include <core/thread.h>
#include <drivers/video/framebuffer/framebuffer.h>
#include <lib/string.h>
#include <stdbool.h>
#include <stdint.h>

static backbuffer_info_t bb_info;
static mutex_t fb_mutex;

color_t black = {0, 0, 0, 255};
color_t gray = {128, 128, 128, 255};
color_t white = {255, 255, 255, 255};
color_t red = {0, 0, 255, 255};
color_t green = {0, 255, 0, 255};
color_t blue = {255, 0, 0, 255};

void fb_test_thread(void *arg) {
    (void)arg;

    // Warte kurz, bis Backbuffer vollstaendig initialisiert ist
    thread_sleep_ms(50);

    uint64_t swidth = fb_get_width();
    uint64_t sheight = fb_get_height();

    int64_t radius = 30;
    int64_t x = radius;
    int64_t y = radius;
    int64_t dx = 8;
    int64_t dy = 8;

    color_t bg_color = {.r = 20, .g = 20, .b = 20, .a = 255};
    color_t circle_color = {.r = 0, .g = 200, .b = 255, .a = 255};
    color_t border_color = {.r = 255, .g = 255, .b = 255, .a = 255};

    while (1) {
        // 1. Backbuffer leeren
        fb_clear(bg_color);

        // 2. Äußeres Rechteck zeichnen (Screen Border)
        fb_draw_rectangle(0, 0, swidth, sheight, border_color, false, 4);

        // 3. Position aktualisieren
        x += dx;
        y += dy;

        // Kollisionsprüfung mit den Bildschirmrändern (Bouncing)
        if (x - radius <= 0) {
            x = radius;
            dx = -dx;
        } else if (x + radius >= (int64_t)swidth) {
            x = swidth - radius;
            dx = -dx;
        }

        if (y - radius <= 0) {
            y = radius;
            dy = -dy;
        } else if (y + radius >= (int64_t)sheight) {
            y = sheight - radius;
            dy = -dy;
        }

        // 4. Kreis zeichnen
        fb_draw_circle(x, y, radius, circle_color, true, 0);

        // 5. ~60 FPS Begrenzung
        thread_sleep_ms(16);
    }

    scheduler_thread_exit();
}

void frame_swap_thread() {
    while (1) {
        mutex_lock(&fb_mutex);
        uint8_t *backbuffer = bb_info.backbuffer;
        memcpy((void *)kernel_fb_info.fb_addr, backbuffer,
               bb_info.backbuffer_size);
        mutex_unlock(&fb_mutex);
        thread_sleep_ms(16);
    }
}

void framebuffer_init_thread() {
    mutex_init(&fb_mutex, "framebuffer_lock");

    size_t backbuffer_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
    serial_printf(COM1,
                  "FB: initializing framebuffer at %llx: %d:%d, %d bpp, pitch: "
                  "%d, backbuffer_size: %d bytes\n",
                  kernel_fb_info.fb_addr, kernel_fb_info.fb_width,
                  kernel_fb_info.fb_height, kernel_fb_info.fb_bpp,
                  kernel_fb_info.fb_pitch, backbuffer_size);
    bb_info.backbuffer = (uint8_t *)kzalloc(backbuffer_size);
    if (!bb_info.backbuffer) {
        serial_printf(COM1, "FB: failed to allocate backbuffer\n");
        return;
    }
    bb_info.backbuffer_size = backbuffer_size;

    serial_printf(COM1, "FB: Backbuffer Virt: %llx, Phys: %llx\n",
                  (virt_addr_t)bb_info.backbuffer,
                  (phys_addr_t)V2P((virt_addr_t)bb_info.backbuffer));

    thread_create(NULL, frame_swap_thread, NULL, "frame_swap_thread");
    thread_create(NULL, fb_test_thread, NULL, "fb_test_thread");

    scheduler_thread_exit();
}

uint64_t fb_get_width() { return kernel_fb_info.fb_width; }

uint64_t fb_get_height() { return kernel_fb_info.fb_height; }

size_t fb_get_backbuffer_size() { return bb_info.backbuffer_size; }

void fb_clear(color_t color) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    if (kernel_fb_info.fb_bpp != 32) {
        serial_printf(COM1,
                      "FB: framebuffer driver only supports 32 bit per pixel");
        return;
    }

    mutex_lock(&fb_mutex);

    uint8_t *backbuffer = bb_info.backbuffer;
    uint64_t width = kernel_fb_info.fb_width;
    uint64_t height = kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    for (uint64_t y = 0; y < height; y++) {
        memset32(backbuffer + (y * pitch), color_val, width);
    }

    mutex_unlock(&fb_mutex);
}

void fb_draw_pixel(uint64_t x, uint64_t y, color_t color) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    if (kernel_fb_info.fb_bpp != 32) {
        serial_printf(COM1,
                      "FB: framebuffer driver only supports 32 bit per pixel");
        return;
    }

    uint8_t *backbuffer = bb_info.backbuffer;
    uint64_t width = kernel_fb_info.fb_width;
    uint64_t height = kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;
    uint64_t bpp_bytes = kernel_fb_info.fb_bpp / 8;

    if (x >= width || y >= height) {
        return;
    }

    mutex_lock(&fb_mutex);

    uint64_t offset = (y * pitch) + (x * bpp_bytes);
    *(uint32_t *)(backbuffer + offset) = color_val;

    mutex_unlock(&fb_mutex);
}

static void fb_draw_rectangle_filled(uint64_t x, uint64_t y, uint64_t width,
                                     uint64_t height, color_t color) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    if (kernel_fb_info.fb_bpp != 32) {
        serial_printf(COM1,
                      "FB: framebuffer driver only supports 32 bit per pixel");
        return;
    }

    uint64_t swidth = kernel_fb_info.fb_width;
    uint64_t sheight = kernel_fb_info.fb_height;

    if (x >= swidth || y >= sheight) {
        return;
    }

    if (x + width > swidth) {
        width = swidth - x;
    }
    if (y + height > sheight) {
        height = sheight - y;
    }

    mutex_lock(&fb_mutex);

    uint8_t *backbuffer = bb_info.backbuffer;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    for (uint64_t i = 0; i < height; i++) {
        memset32(backbuffer + ((y + i) * pitch) + (x * 4), color_val, width);
    }

    mutex_unlock(&fb_mutex);
}

static void fb_draw_rectangle_outline(uint64_t x, uint64_t y, uint64_t width,
                                      uint64_t height, color_t color,
                                      uint64_t border_size) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    if (kernel_fb_info.fb_bpp != 32) {
        serial_printf(COM1,
                      "FB: framebuffer driver only supports 32 bit per pixel");
        return;
    }

    uint8_t *backbuffer = bb_info.backbuffer;
    uint64_t swidth = kernel_fb_info.fb_width;
    uint64_t sheight = kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;
    uint64_t bpp_bytes = kernel_fb_info.fb_bpp / 8;

    if (x >= swidth || y >= sheight) {
        return;
    }

    if (x + width > swidth) {
        width = swidth - x;
    }

    if (y + height > sheight) {
        height = sheight - y;
    }

    if (border_size > width)
        border_size = width;
    if (border_size > height)
        border_size = height;

    mutex_lock(&fb_mutex);

    for (uint64_t i = 0; i < border_size; i++) {
        memset32(backbuffer + ((y + i) * pitch) + (x * bpp_bytes), color_val,
                 width);
    }

    for (uint64_t i = 0; i < border_size; i++) {
        memset32(backbuffer + ((y + height - border_size + i) * pitch) +
                     (x * bpp_bytes),
                 color_val, width);
    }

    for (uint64_t i = border_size; i < height - border_size; i++) {
        memset32(backbuffer + ((y + i) * pitch) + (x * bpp_bytes), color_val,
                 border_size);
        memset32(backbuffer + ((y + i) * pitch) +
                     ((x + width - border_size) * bpp_bytes),
                 color_val, border_size);
    }

    mutex_unlock(&fb_mutex);
}

void fb_draw_rectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                       color_t color, bool filled, uint64_t border_size) {
    if (filled) {
        fb_draw_rectangle_filled(x, y, width, height, color);
    } else {
        fb_draw_rectangle_outline(x, y, width, height, color, border_size);
    }
}

static void fb_draw_circle_filled(uint64_t xc, uint64_t yc, uint64_t radius,
                                  color_t color) {
    uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                         ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);

    if (kernel_fb_info.fb_bpp != 32) {
        serial_printf(COM1,
                      "FB: framebuffer driver only supports 32 bit per pixel");
        return;
    }

    uint64_t swidth = kernel_fb_info.fb_width;
    uint64_t sheight = kernel_fb_info.fb_height;
    uint8_t *backbuffer = bb_info.backbuffer;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    mutex_lock(&fb_mutex);

    int64_t x = 0;
    int64_t y = radius;
    int64_t d = 3 - (2 * (int64_t)radius);

#define DRAW_HORIZONTAL_LINE(x_start, x_end, y_pos)                            \
    do {                                                                       \
        int64_t _y = (y_pos);                                                  \
        if (_y >= 0 && _y < (int64_t)sheight) {                                \
            int64_t _xs = (x_start);                                           \
            int64_t _xe = (x_end);                                             \
            if (_xs < 0)                                                       \
                _xs = 0;                                                       \
            if (_xe >= (int64_t)swidth)                                        \
                _xe = (int64_t)swidth - 1;                                     \
            if (_xs <= _xe) {                                                  \
                memset32(backbuffer + (_y * pitch) + (_xs * 4), color_val,     \
                         _xe - _xs + 1);                                       \
            }                                                                  \
        }                                                                      \
    } while (0)

    while (x <= y) {
        DRAW_HORIZONTAL_LINE((int64_t)xc - x, (int64_t)xc + x, (int64_t)yc + y);
        DRAW_HORIZONTAL_LINE((int64_t)xc - x, (int64_t)xc + x, (int64_t)yc - y);
        DRAW_HORIZONTAL_LINE((int64_t)xc - y, (int64_t)xc + y, (int64_t)yc + x);
        DRAW_HORIZONTAL_LINE((int64_t)xc - y, (int64_t)xc + y, (int64_t)yc - x);

        if (d < 0) {
            d = d + (4 * x) + 6;
        } else {
            d = d + (4 * (x - y)) + 10;
            y--;
        }
        x++;
    }

#undef DRAW_HORIZONTAL_LINE
    mutex_unlock(&fb_mutex);
}

static void fb_draw_circle_outline(uint64_t xc, uint64_t yc, uint64_t radius,
                                   color_t color, uint64_t border_size) {
    if (border_size >= radius) {
        fb_draw_circle_filled(xc, yc, radius, color);
        return;
    }

    mutex_lock(&fb_mutex);

    for (uint64_t r = radius - border_size + 1; r <= radius; r++) {
        uint32_t color_val = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                             ((uint32_t)color.r << 16) |
                             ((uint32_t)color.a << 24);

        if (kernel_fb_info.fb_bpp != 32)
            return;

        uint64_t swidth = kernel_fb_info.fb_width;
        uint64_t sheight = kernel_fb_info.fb_height;
        uint8_t *backbuffer = bb_info.backbuffer;
        uint64_t pitch = kernel_fb_info.fb_pitch;

        int64_t x = 0;
        int64_t y = r;
        int64_t d = 3 - (2 * (int64_t)r);

#define PLOT_PIXEL(px, py)                                                     \
    do {                                                                       \
        int64_t _x = (px);                                                     \
        int64_t _y = (py);                                                     \
        if (_x >= 0 && _x < (int64_t)swidth && _y >= 0 &&                      \
            _y < (int64_t)sheight) {                                           \
            *(uint32_t *)(backbuffer + (_y * pitch) + (_x * 4)) = color_val;   \
        }                                                                      \
    } while (0)

        while (x <= y) {
            PLOT_PIXEL((int64_t)xc + x, (int64_t)yc + y);
            PLOT_PIXEL((int64_t)xc - x, (int64_t)yc + y);
            PLOT_PIXEL((int64_t)xc + x, (int64_t)yc - y);
            PLOT_PIXEL((int64_t)xc - x, (int64_t)yc - y);
            PLOT_PIXEL((int64_t)xc + y, (int64_t)yc + x);
            PLOT_PIXEL((int64_t)xc - y, (int64_t)yc + x);
            PLOT_PIXEL((int64_t)xc + y, (int64_t)yc - x);
            PLOT_PIXEL((int64_t)xc - y, (int64_t)yc - x);

            if (d < 0) {
                d = d + (4 * x) + 6;
            } else {
                d = d + (4 * (x - y)) + 10;
                y--;
            }
            x++;
        }

#undef PLOT_PIXEL
    }

    mutex_unlock(&fb_mutex);
}

void fb_draw_circle(uint64_t x, uint64_t y, uint64_t radius, color_t color,
                    bool filled, uint64_t border_size) {
    if (filled) {
        fb_draw_circle_filled(x, y, radius, color);
    } else {
        fb_draw_circle_outline(x, y, radius, color, border_size);
    }
}

void fb_draw_triangle(uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2,
                      uint64_t x3, uint64_t y3, color_t color, bool filled,
                      uint64_t border_size);
void fb_draw_unicode(uint64_t x, uint64_t y, uint32_t unicode, color_t color);
void fb_draw_ustring(uint64_t x, uint64_t y, uint32_t *str, color_t color);
void fb_draw_char(uint64_t x, uint64_t y, char c, color_t color);
void fb_draw_string(uint64_t x, uint64_t y, const char *str, color_t color);
void fb_swap_buffers();