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
#include <stdint.h>

static backbuffer_info_t bb_info;
static mutex_t fb_mutex;

color_t black = {0, 0, 0, 255};
color_t gray = {128, 128, 128, 255};
color_t white = {255, 255, 255, 255};
color_t red = {0, 0, 255, 255};
color_t green = {0, 255, 0, 255};
color_t blue = {255, 0, 0, 255};

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

    size_t backbuffer_size =
        kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
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

    while (1) {
        fb_clear(red);
        thread_sleep_ms(500);
        fb_clear(green);
        thread_sleep_ms(500);
        fb_clear(blue);
        thread_sleep_ms(500);
    }

    scheduler_thread_exit();
}

uint64_t fb_get_width() { return kernel_fb_info.fb_width; }

uint64_t fb_get_height() { return kernel_fb_info.fb_height; }

size_t fb_get_backbuffer_size() { return bb_info.backbuffer_size; }

static inline int fb_prepare_color(color_t color, uint32_t *out_color) {
    if (kernel_fb_info.fb_bpp == 32) {
        *out_color = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                     ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);
        return 4;
    } else if (kernel_fb_info.fb_bpp == 24) {
        *out_color = ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
                     ((uint32_t)color.r << 16);
        return 3;
    } else {
        return 0;
    }
}

void fb_clear(color_t color) {
    uint32_t color_val = 0;
    int bpp_bytes = fb_prepare_color(color, &color_val);

    if (bpp_bytes == 0) {
        serial_printf(
            COM1,
            "FB: framebuffer driver only supports 24 or 32 bit per pixel");
        return;
    }

    mutex_lock(&fb_mutex);

    uint8_t *backbuffer = bb_info.backbuffer;
    uint64_t width = kernel_fb_info.fb_width;
    uint64_t height = kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    if (bpp_bytes == 4) {
        for (uint64_t y = 0; y < height; y++) {
            memset32(backbuffer + (y * pitch), color_val, width);
        }
    } else {
        if (color.r == color.g && color.g == color.b) {
            for (uint64_t y = 0; y < height; y++) {
                memset(backbuffer + (y * pitch), color.r, width * 3);
            }
        } else {
            for (uint64_t y = 0; y < height; y++) {
                uint8_t *row = backbuffer + (y * pitch);
                for (uint64_t x = 0; x < width; x++) {
                    row[x * 3 + 0] = color.b;
                    row[x * 3 + 1] = color.g;
                    row[x * 3 + 2] = color.r;
                }
            }
        }
    }

    mutex_unlock(&fb_mutex);
}

void fb_draw_pixel(uint64_t x, uint64_t y, color_t color);
void fb_draw_rectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                       color_t color, bool filled, uint64_t border_size);
void fb_draw_circle(uint64_t x, uint64_t y, uint64_t radius, color_t color,
                    bool filled, uint64_t border_size);
void fb_draw_triangle(uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2,
                      uint64_t x3, uint64_t y3, color_t color, bool filled,
                      uint64_t border_size);
void fb_draw_unicode(uint64_t x, uint64_t y, uint32_t unicode, color_t color);
void fb_draw_ustring(uint64_t x, uint64_t y, uint32_t *str, color_t color);
void fb_draw_char(uint64_t x, uint64_t y, char c, color_t color);
void fb_draw_string(uint64_t x, uint64_t y, const char *str, color_t color);
void fb_swap_buffers();