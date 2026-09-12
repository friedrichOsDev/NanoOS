/**
 * @file framebuffer.c
 * @brief Framebuffer driver with Canvas & Layer Rendering
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
static bool fb_initialized = false;
static dirty_rect_t global_dirty_rect = {0, 0, 0, 0, false};

static inline uint32_t pack_color(color_t color) {
    return ((uint32_t)color.b) | ((uint32_t)color.g << 8) |
           ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);
}

// Optimized Alpha-Over Blending algorithm
static inline uint32_t alpha_blend(uint32_t src, uint32_t dst) {
    uint32_t alpha = (src >> 24) & 0xFF;

    if (alpha == 255)
        return src;
    if (alpha == 0)
        return dst;

    uint32_t inv_alpha = 255 - alpha;

    uint32_t src_rb = src & 0x00FF00FF;
    uint32_t src_g = src & 0x0000FF00;

    uint32_t dst_rb = dst & 0x00FF00FF;
    uint32_t dst_g = dst & 0x0000FF00;

    uint32_t res_rb = ((src_rb * alpha + dst_rb * inv_alpha) >> 8) & 0x00FF00FF;
    uint32_t res_g = ((src_g * alpha + dst_g * inv_alpha) >> 8) & 0x0000FF00;

    return 0xFF000000 | res_rb | res_g;
}

static inline void mark_global_dirty(int64_t x1, int64_t y1, int64_t x2, int64_t y2) {
    if (!global_dirty_rect.is_dirty) {
        global_dirty_rect.x1 = x1;
        global_dirty_rect.y1 = y1;
        global_dirty_rect.x2 = x2;
        global_dirty_rect.y2 = y2;
        global_dirty_rect.is_dirty = true;
    } else {
        if (x1 < global_dirty_rect.x1)
            global_dirty_rect.x1 = x1;
        if (y1 < global_dirty_rect.y1)
            global_dirty_rect.y1 = y1;
        if (x2 > global_dirty_rect.x2)
            global_dirty_rect.x2 = x2;
        if (y2 > global_dirty_rect.y2)
            global_dirty_rect.y2 = y2;
    }
}

void canvas_mark_dirty(canvas_t *canvas, int64_t x1, int64_t y1, int64_t x2, int64_t y2) {
    if (!canvas)
        return;
    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (x2 >= (int64_t)canvas->width)
        x2 = canvas->width - 1;
    if (y2 >= (int64_t)canvas->height)
        y2 = canvas->height - 1;

    if (!canvas->dirty.is_dirty) {
        canvas->dirty.x1 = x1;
        canvas->dirty.y1 = y1;
        canvas->dirty.x2 = x2;
        canvas->dirty.y2 = y2;
        canvas->dirty.is_dirty = true;
    } else {
        if (x1 < canvas->dirty.x1)
            canvas->dirty.x1 = x1;
        if (y1 < canvas->dirty.y1)
            canvas->dirty.y1 = y1;
        if (x2 > canvas->dirty.x2)
            canvas->dirty.x2 = x2;
        if (y2 > canvas->dirty.y2)
            canvas->dirty.y2 = y2;
    }
}

uint64_t fb_get_width(void) { return kernel_fb_info.fb_width; }
uint64_t fb_get_height(void) { return kernel_fb_info.fb_height; }

// ==========
// Canvas API
// ==========

canvas_t *canvas_create(uint64_t width, uint64_t height) {
    canvas_t *canvas = (canvas_t *)kzalloc(sizeof(canvas_t));
    if (!canvas)
        return NULL;

    canvas->width = width;
    canvas->height = height;
    canvas->pitch = width * 4;
    canvas->buffer = (uint32_t *)kzalloc(height * canvas->pitch);
    canvas->dirty = (dirty_rect_t){0, 0, 0, 0, false};

    if (!canvas->buffer) {
        kfree((virt_addr_t)canvas);
        return NULL;
    }
    return canvas;
}

void canvas_destroy(canvas_t *canvas) {
    if (!canvas)
        return;
    if (canvas->buffer)
        kfree((virt_addr_t)(canvas->buffer));
    kfree((virt_addr_t)canvas);
}

void canvas_clear(canvas_t *canvas, color_t color) {
    if (!canvas || !canvas->buffer)
        return;
    uint32_t color_val = pack_color(color);
    size_t count = canvas->width * canvas->height;

    memset32((uint8_t *)canvas->buffer, color_val, count);
    canvas_mark_dirty(canvas, 0, 0, canvas->width - 1, canvas->height - 1);
}

void canvas_draw_pixel(canvas_t *canvas, uint64_t x, uint64_t y, color_t color) {
    if (!canvas || x >= canvas->width || y >= canvas->height)
        return;
    canvas->buffer[y * canvas->width + x] = pack_color(color);
    canvas_mark_dirty(canvas, x, y, x, y);
}

void canvas_draw_rectangle(canvas_t *canvas, uint64_t x, uint64_t y, uint64_t width,
                           uint64_t height, color_t color, bool filled, uint64_t border_size) {
    if (!canvas)
        return;
    if (x >= canvas->width || y >= canvas->height)
        return;
    if (x + width > canvas->width)
        width = canvas->width - x;
    if (y + height > canvas->height)
        height = canvas->height - y;

    uint32_t color_val = pack_color(color);

    if (filled) {
        for (uint64_t i = 0; i < height; i++) {
            memset32((uint8_t *)(canvas->buffer + ((y + i) * canvas->width + x)),
                     color_val, width);
        }
    } else {
        if (border_size > width)
            border_size = width;
        if (border_size > height)
            border_size = height;

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
    canvas_mark_dirty(canvas, x, y, x + width - 1, y + height - 1);
}

void canvas_draw_line(canvas_t *canvas, int64_t x1, int64_t y1, int64_t x2, int64_t y2,
                      color_t color, uint64_t thickness) {
    if (!canvas || thickness == 0)
        return;

    uint32_t color_val = pack_color(color);
    double dx = (double)(x2 - x1);
    double dy = (double)(y2 - y1);
    double length = hypot(dx, dy);

    if (length == 0.0) {
        if (x1 >= 0 && x1 < (int64_t)canvas->width && y1 >= 0 && y1 < (int64_t)canvas->height) {
            canvas->buffer[y1 * canvas->width + x1] = color_val;
            canvas_mark_dirty(canvas, x1, y1, x1, y1);
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

    int64_t min_x = x1 < x2 ? x1 : x2;
    int64_t max_x = x1 > x2 ? x1 : x2;
    int64_t min_y = y1 < y2 ? y1 : y2;
    int64_t max_y = y1 > y2 ? y1 : y2;
    int64_t pad = (int64_t)(thickness / 2);
    canvas_mark_dirty(canvas, min_x - pad, min_y - pad, max_x + pad, max_y + pad);
}

void canvas_draw_circle(canvas_t *canvas, uint64_t xc, uint64_t yc, uint64_t radius,
                        color_t color, bool filled, uint64_t border_size) {
    if (!canvas)
        return;
    uint32_t color_val = pack_color(color);

    if (filled) {
        int64_t r = (int64_t)radius;
        for (int64_t dy = -r; dy <= r; dy++) {
            int64_t yp = (int64_t)yc + dy;
            if (yp < 0 || yp >= (int64_t)canvas->height)
                continue;

            int64_t dx = (int64_t)sqrt(r * r - dy * dy);
            int64_t xs = (int64_t)xc - dx;
            int64_t xe = (int64_t)xc + dx;

            if (xs < 0)
                xs = 0;
            if (xe >= (int64_t)canvas->width)
                xe = (int64_t)canvas->width - 1;

            if (xs <= xe) {
                memset32((uint8_t *)(canvas->buffer + (yp * canvas->width + xs)), color_val, xe - xs + 1);
            }
        }
    } else {
        int64_t r_outer = (int64_t)radius;
        int64_t r_inner = (int64_t)(radius > border_size ? radius - border_size : 0);

        for (int64_t dy = -r_outer; dy <= r_outer; dy++) {
            int64_t yp = (int64_t)yc + dy;
            if (yp < 0 || yp >= (int64_t)canvas->height)
                continue;

            for (int64_t dx = -r_outer; dx <= r_outer; dx++) {
                int64_t xp = (int64_t)xc + dx;
                if (xp < 0 || xp >= (int64_t)canvas->width)
                    continue;

                double dist = hypot((double)dx, (double)dy);
                if (dist <= r_outer && dist >= r_inner) {
                    canvas->buffer[yp * canvas->width + xp] = color_val;
                }
            }
        }
    }
    canvas_mark_dirty(canvas, xc - radius, yc - radius, xc + radius, yc + radius);
}

// =========================
// Layer / Scene Manager API
// =========================

layer_t *layer_create(uint64_t width, uint64_t height, int32_t z_index) {
    layer_t *layer = (layer_t *)kzalloc(sizeof(layer_t));
    if (!layer)
        return NULL;

    canvas_t *c = canvas_create(width, height);
    if (!c) {
        kfree((virt_addr_t)layer);
        return NULL;
    }

    layer->canvas = *c;
    kfree((virt_addr_t)c);
    layer->x = 0;
    layer->y = 0;
    layer->z_index = z_index;
    layer->visible = true;

    mutex_lock(&fb_scene_mutex);

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

    mark_global_dirty(0, 0, kernel_fb_info.fb_width - 1, kernel_fb_info.fb_height - 1);
    mutex_unlock(&fb_scene_mutex);
    return layer;
}

void layer_destroy(layer_t *layer) {
    if (!layer)
        return;

    mutex_lock(&fb_scene_mutex);
    if (layer_head == layer) {
        layer_head = layer->next;
    } else {
        layer_t *curr = layer_head;
        while (curr && curr->next != layer) {
            curr = curr->next;
        }
        if (curr)
            curr->next = layer->next;
    }

    mark_global_dirty(0, 0, kernel_fb_info.fb_width - 1, kernel_fb_info.fb_height - 1);
    mutex_unlock(&fb_scene_mutex);

    if (layer->canvas.buffer)
        kfree((virt_addr_t)(layer->canvas.buffer));
    kfree((virt_addr_t)layer);
}

void layer_set_position(layer_t *layer, int64_t x, int64_t y) {
    if (!layer)
        return;

    if (layer->x != x || layer->y != y) {
        mutex_lock(&fb_scene_mutex);

        if (layer->canvas.width > 0 && layer->canvas.height > 0) {
            mark_global_dirty(layer->x, layer->y,
                              layer->x + (int64_t)layer->canvas.width - 1,
                              layer->y + (int64_t)layer->canvas.height - 1);
        }

        layer->x = x;
        layer->y = y;

        if (layer->canvas.width > 0 && layer->canvas.height > 0) {
            mark_global_dirty(layer->x, layer->y,
                              layer->x + (int64_t)layer->canvas.width - 1,
                              layer->y + (int64_t)layer->canvas.height - 1);
        }
        mutex_unlock(&fb_scene_mutex);
    }
}

void layer_set_visible(layer_t *layer, bool visible) {
    if (layer && layer->visible != visible) {
        layer->visible = visible;
        mutex_lock(&fb_scene_mutex);
        mark_global_dirty(layer->x, layer->y, layer->x + layer->canvas.width, layer->y + layer->canvas.height);
        mutex_unlock(&fb_scene_mutex);
    }
}

// =================
// Compositor Engine
// =================

static void composite_layer_to_backbuffer(layer_t *layer, dirty_rect_t *dirty) {
    if (!layer || !layer->visible || !layer->canvas.buffer)
        return;

    canvas_t *c = &layer->canvas;
    int64_t screen_w = (int64_t)kernel_fb_info.fb_width;
    int64_t screen_h = (int64_t)kernel_fb_info.fb_height;
    uint64_t pitch = kernel_fb_info.fb_pitch;

    int64_t layer_max_x = layer->x + (int64_t)c->width - 1;
    int64_t layer_max_y = layer->y + (int64_t)c->height - 1;

    int64_t start_y = dirty->y1 > layer->y ? dirty->y1 : layer->y;
    int64_t end_y = dirty->y2 < layer_max_y ? dirty->y2 : layer_max_y;

    int64_t start_x = dirty->x1 > layer->x ? dirty->x1 : layer->x;
    int64_t end_x = dirty->x2 < layer_max_x ? dirty->x2 : layer_max_x;

    if (start_y < 0)
        start_y = 0;
    if (end_y >= screen_h)
        end_y = screen_h - 1;
    if (start_x < 0)
        start_x = 0;
    if (end_x >= screen_w)
        end_x = screen_w - 1;

    if (start_y > end_y || start_x > end_x)
        return;

    for (int64_t sy = start_y; sy <= end_y; sy++) {
        int64_t cy = sy - layer->y;
        if (cy < 0 || cy >= (int64_t)c->height)
            continue;

        uint8_t *row_dst = fb_back_buf + (sy * pitch);

        for (int64_t sx = start_x; sx <= end_x; sx++) {
            int64_t cx = sx - layer->x;
            if (cx < 0 || cx >= (int64_t)c->width)
                continue;

            uint32_t src_color = c->buffer[cy * c->width + cx];

            if ((src_color >> 24) == 0)
                continue;

            uint32_t *dst_pixel = (uint32_t *)(row_dst + (sx * 4));
            *dst_pixel = alpha_blend(src_color, *dst_pixel);
        }
    }
}

void fb_render_scene(void) {
    if (!fb_initialized)
        return;

    mutex_lock(&fb_scene_mutex);

    // Collect dirty region from canvases
    layer_t *curr = layer_head;
    while (curr) {
        if (curr->visible && curr->canvas.dirty.is_dirty) {
            mark_global_dirty(curr->x + curr->canvas.dirty.x1,
                              curr->y + curr->canvas.dirty.y1,
                              curr->x + curr->canvas.dirty.x2,
                              curr->y + curr->canvas.dirty.y2);
            curr->canvas.dirty.is_dirty = false;
        }
        curr = curr->next;
    }

    if (!global_dirty_rect.is_dirty) {
        mutex_unlock(&fb_scene_mutex);
        return;
    }

    // Clamp dirty rect to screen bounds
    if (global_dirty_rect.x1 < 0)
        global_dirty_rect.x1 = 0;
    if (global_dirty_rect.y1 < 0)
        global_dirty_rect.y1 = 0;
    if (global_dirty_rect.x2 >= (int64_t)kernel_fb_info.fb_width)
        global_dirty_rect.x2 = kernel_fb_info.fb_width - 1;
    if (global_dirty_rect.y2 >= (int64_t)kernel_fb_info.fb_height)
        global_dirty_rect.y2 = kernel_fb_info.fb_height - 1;

    // 1. Clear dirty region in Backbuffer (Korrektes Pixel-Filling)
    uint32_t bg_color = pack_color(black);
    uint64_t pitch = kernel_fb_info.fb_pitch;

    for (int64_t y = global_dirty_rect.y1; y <= global_dirty_rect.y2; y++) {
        uint32_t *row = (uint32_t *)(fb_back_buf + (y * pitch));
        for (int64_t x = global_dirty_rect.x1; x <= global_dirty_rect.x2; x++) {
            row[x] = bg_color;
        }
    }

    // 2. Render overlapping visible layers inside dirty rect
    curr = layer_head;
    while (curr) {
        composite_layer_to_backbuffer(curr, &global_dirty_rect);
        curr = curr->next;
    }

    // 3. Flush ONLY dirty region to physical VRAM
    if (fb_back_buf && kernel_fb_info.fb_addr) {
        uint8_t *dst_vram = (uint8_t *)kernel_fb_info.fb_addr;
        uint64_t width_to_clear = global_dirty_rect.x2 - global_dirty_rect.x1 + 1;

        if (width_to_clear == kernel_fb_info.fb_width) {
            uint64_t start_offset = global_dirty_rect.y1 * pitch;
            uint64_t total_bytes = (global_dirty_rect.y2 - global_dirty_rect.y1 + 1) * pitch;
            memcpy(dst_vram + start_offset, fb_back_buf + start_offset, total_bytes);
        } else {
            size_t bytes_per_line = width_to_clear * 4;
            for (int64_t y = global_dirty_rect.y1; y <= global_dirty_rect.y2; y++) {
                uint64_t offset = (y * pitch) + (global_dirty_rect.x1 * 4);
                memcpy(dst_vram + offset, fb_back_buf + offset, bytes_per_line);
            }
        }
    }

    // Reset dirty rect
    global_dirty_rect.is_dirty = false;

    mutex_unlock(&fb_scene_mutex);
}

void fb_init(void) {
    fb_buffer_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;

    serial_printf(COM1, "FB: Initializing ...\n");

    mutex_init(&fb_scene_mutex, "FB Scene Mutex");
    fb_back_buf = (uint8_t *)kzalloc(fb_buffer_size);

    if (!fb_back_buf) {
        serial_printf(COM1, "FB: Allocation failed!\n");
        return;
    }

    fb_initialized = true;
}
