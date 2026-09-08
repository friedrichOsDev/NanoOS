/**
 * @file framebuffer.c
 * @brief Framebuffer driver with Canvas & Layer Rendering
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/memdef.h>
#include <core/init.h>
#include <core/sync.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <drivers/video/framebuffer/framebuffer.h>
#include <lib/math/math.h>
#include <lib/string.h>
#include <stdbool.h>
#include <stdint.h>

color_t black = {0, 0, 0, 255};
color_t gray = {128, 128, 128, 255};
color_t white = {255, 255, 255, 255};
color_t red = {0, 0, 255, 255};
color_t green = {0, 255, 0, 255};
color_t blue = {255, 0, 0, 255};

static uint8_t *fb_back_buf = NULL;
static size_t fb_buffer_size = 0;
static mutex_t fb_scene_mutex;
static layer_t *layer_head = NULL;

static inline uint32_t pack_color(color_t color) {
    return ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
           ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);
}

uint64_t fb_get_width(void) { return kernel_fb_info.fb_width; }
uint64_t fb_get_height(void) { return kernel_fb_info.fb_height; }

// Canvas API
canvas_t *canvas_create(uint64_t width, uint64_t height) {
    canvas_t *canvas = (canvas_t *)kzalloc(sizeof(canvas_t));
    if (!canvas) return NULL;

    canvas->width = width;
    canvas->height = height;
    canvas->pitch = width * 4;
    canvas->buffer = (uint32_t *)kzalloc(height * canvas->pitch);

    if (!canvas->buffer) {
        kfree((virt_addr_t)canvas);
        return NULL;
    }
    return canvas;
}

void canvas_destroy(canvas_t *canvas) {
    if (!canvas) return;
    if (canvas->buffer) kfree((virt_addr_t)(canvas->buffer));
    kfree((virt_addr_t)canvas);
}

void canvas_clear(canvas_t *canvas, color_t color) {
    if (!canvas || !canvas->buffer) return;
    uint32_t color_val = pack_color(color);
    size_t count = canvas->width * canvas->height;

    memset32((uint8_t *)canvas->buffer, color_val, count);
}

void canvas_draw_pixel(canvas_t *canvas, uint64_t x, uint64_t y, color_t color) {
    if (!canvas || x >= canvas->width || y >= canvas->height) return;
    canvas->buffer[y * canvas->width + x] = pack_color(color);
}

void canvas_draw_rectangle(canvas_t *canvas, uint64_t x, uint64_t y, uint64_t width,
                          uint64_t height, color_t color, bool filled, uint64_t border_size) {
    if (!canvas) return;
    if (x >= canvas->width || y >= canvas->height) return;
    if (x + width > canvas->width) width = canvas->width - x;
    if (y + height > canvas->height) height = canvas->height - y;

    uint32_t color_val = pack_color(color);

    if (filled) {
        for (uint64_t i = 0; i < height; i++) {
            memset32((uint8_t *)(canvas->buffer + ((y + i) * canvas->width + x)),
                     color_val, width);
        }
    } else {
        if (border_size > width) border_size = width;
        if (border_size > height) border_size = height;

        for (uint64_t i = 0; i < border_size; i++) {
            memset32((uint8_t *)(canvas->buffer + ((y + i) * canvas->width + x)),
                     color_val, width);
            memset32((uint8_t *)(canvas->buffer + ((y + height - border_size + i) * canvas->width + x)),
                     color_val, width);
        }
        for (uint64_t i = border_size; i < height - border_size; i++) {
            memset32((uint8_t *)(canvas->buffer + ((y + i) * canvas->width + x)),
                     color_val, border_size);
            memset32((uint8_t *)(canvas->buffer + ((y + i) * canvas->width + (x + width - border_size))),
                     color_val, border_size);
        }
    }
}

void canvas_draw_line(canvas_t *canvas, int64_t x1, int64_t y1, int64_t x2, int64_t y2,
                      color_t color, uint64_t thickness) {
    if (!canvas || thickness == 0) return;

    uint32_t color_val = pack_color(color);
    double dx = (double)(x2 - x1);
    double dy = (double)(y2 - y1);
    double length = hypot(dx, dy);

    if (length == 0.0) {
        if (x1 >= 0 && x1 < (int64_t)canvas->width && y1 >= 0 && y1 < (int64_t)canvas->height) {
            canvas->buffer[y1 * canvas->width + x1] = color_val;
        }
        return;
    }

    double ux = dx / length;
    double uy = dy / length;

    for (double i = 0; i <= length; i += 0.5) {
        int64_t cx = (int64_t)round((double)x1 + ux * i);
        int64_t cy = (int64_t)round((double)y1 + uy * i);

        if (thickness == 1) {
            if (cx >= 0 && cx < (int64_t)canvas->width && cy >= 0 && cy < (int64_t)canvas->height) {
                canvas->buffer[cy * canvas->width + cx] = color_val;
            }
        } else {
            int64_t r = (int64_t)(thickness / 2);
            for (int64_t tx = -r; tx <= r; tx++) {
                for (int64_t ty = -r; ty <= r; ty++) {
                    int64_t px = cx + tx;
                    int64_t py = cy + ty;
                    if (px >= 0 && px < (int64_t)canvas->width && py >= 0 && py < (int64_t)canvas->height) {
                        canvas->buffer[py * canvas->width + px] = color_val;
                    }
                }
            }
        }
    }
}

void canvas_draw_circle(canvas_t *canvas, uint64_t xc, uint64_t yc, uint64_t radius,
                        color_t color, bool filled, uint64_t border_size) {
    if (!canvas) return;
    uint32_t color_val = pack_color(color);

    if (filled) {
        int64_t r = (int64_t)radius;
        for (int64_t dy = -r; dy <= r; dy++) {
            int64_t yp = (int64_t)yc + dy;
            if (yp < 0 || yp >= (int64_t)canvas->height) continue;

            int64_t dx = (int64_t)sqrt(r * r - dy * dy);
            int64_t xs = (int64_t)xc - dx;
            int64_t xe = (int64_t)xc + dx;

            if (xs < 0) xs = 0;
            if (xe >= (int64_t)canvas->width) xe = (int64_t)canvas->width - 1;

            if (xs <= xe) {
                memset32((uint8_t *)(canvas->buffer + (yp * canvas->width + xs)), color_val, xe - xs + 1);
            }
        }
    } else {
        int64_t r_outer = (int64_t)radius;
        int64_t r_inner = (int64_t)(radius > border_size ? radius - border_size : 0);

        for (int64_t dy = -r_outer; dy <= r_outer; dy++) {
            int64_t yp = (int64_t)yc + dy;
            if (yp < 0 || yp >= (int64_t)canvas->height) continue;

            for (int64_t dx = -r_outer; dx <= r_outer; dx++) {
                int64_t xp = (int64_t)xc + dx;
                if (xp < 0 || xp >= (int64_t)canvas->width) continue;

                double dist = hypot((double)dx, (double)dy);
                if (dist <= r_outer && dist >= r_inner) {
                    canvas->buffer[yp * canvas->width + xp] = color_val;
                }
            }
        }
    }
}

//Layer / Scene Manager API
layer_t *layer_create(uint64_t width, uint64_t height, int32_t z_index) {
    layer_t *layer = (layer_t *)kzalloc(sizeof(layer_t));
    if (!layer) return NULL;

    canvas_t *c = canvas_create(width, height);
    if (!c) {
        kfree((virt_addr_t)layer);
        return NULL;
    }

    layer->canvas = *c;
    kfree((virt_addr_t)c); // Structure content copied
    layer->x = 0;
    layer->y = 0;
    layer->z_index = z_index;
    layer->visible = true;

    mutex_lock(&fb_scene_mutex);

    // Insert sorted by z_index
    if (!layer_head || layer_head->z_index > z_index) {
        layer->next = layer_head;
        layer_head = layer;
    } else {
        layer_t *curr = layer_head;
        while (curr->next && curr->next->z_index <= z_index) {
            curr = curr->next;
        }
        layer->next = curr->next;
        curr->next = layer;
    }

    mutex_unlock(&fb_scene_mutex);
    return layer;
}

void layer_destroy(layer_t *layer) {
    if (!layer) return;

    mutex_lock(&fb_scene_mutex);
    if (layer_head == layer) {
        layer_head = layer->next;
    } else {
        layer_t *curr = layer_head;
        while (curr && curr->next != layer) {
            curr = curr->next;
        }
        if (curr) curr->next = layer->next;
    }
    mutex_unlock(&fb_scene_mutex);

    if (layer->canvas.buffer) kfree((virt_addr_t)(layer->canvas.buffer));
    kfree((virt_addr_t)layer);
}

void layer_set_position(layer_t *layer, int64_t x, int64_t y) {
    if (layer) {
        layer->x = x;
        layer->y = y;
    }
}

void layer_set_visible(layer_t *layer, bool visible) {
    if (layer) layer->visible = visible;
}

// Compositor & Driver Engine
static void composite_layer_to_backbuffer(layer_t *layer) {
    if (!layer || !layer->visible) return;

    canvas_t *c = &layer->canvas;
    int64_t screen_w = (int64_t)kernel_fb_info.fb_width;
    int64_t screen_h = (int64_t)kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    for (int64_t cy = 0; cy < (int64_t)c->height; cy++) {
        int64_t sy = layer->y + cy;
        if (sy < 0 || sy >= screen_h) continue;

        for (int64_t cx = 0; cx < (int64_t)c->width; cx++) {
            int64_t sx = layer->x + cx;
            if (sx < 0 || sx >= screen_w) continue;

            uint32_t color_val = c->buffer[cy * c->width + cx];
            uint8_t alpha = (color_val >> 24) & 0xFF;

            // Simple transparency check (0 = Fully transparent)
            if (alpha == 0) continue;

            *(uint32_t *)(fb_back_buf + (sy * pitch) + (sx * 4)) = color_val;
        }
    }
}

void fb_render_scene(void) {
    mutex_lock(&fb_scene_mutex);

    if (!fb_back_buf || !kernel_fb_info.fb_addr) {
        mutex_unlock(&fb_scene_mutex);
        return;
    }

    // 1. Clear Backbuffer with background color
    uint32_t bg_color = pack_color(black);
    for (uint64_t y = 0; y < kernel_fb_info.fb_height; y++) {
        memset32(fb_back_buf + (y * kernel_fb_info.fb_pitch), bg_color, kernel_fb_info.fb_width);
    }

    // 2. Render all visible layers sorted by Z-index
    layer_t *curr = layer_head;
    while (curr) {
        composite_layer_to_backbuffer(curr);
        curr = curr->next;
    }

    // 3. Swap/Copy Backbuffer to Physical Framebuffer (VRAM)
    if (fb_back_buf && kernel_fb_info.fb_addr) {
        memcpy((void *)kernel_fb_info.fb_addr, fb_back_buf, fb_buffer_size);
    }

    mutex_unlock(&fb_scene_mutex);
}

void framebuffer_init_thread(void *arg) {
    (void)arg;
    fb_buffer_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;

    serial_printf(COM1, "FB: Initializing ...\n");

    mutex_init(&fb_scene_mutex, "FB Scene Mutex");
    fb_back_buf = (uint8_t *)kzalloc(fb_buffer_size);

    if (!fb_back_buf) {
        serial_printf(COM1, "FB: Allocation failed!\n");
        return;
    }

    scheduler_thread_exit();
}