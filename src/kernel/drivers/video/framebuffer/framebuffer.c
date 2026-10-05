/**
 * @file framebuffer.c
 * @brief Thread-sichere Implementation des Framebuffer-Treibers.
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/mm/memdef.h>
#include <core/scheduler.h>
#include <arch/x86_64/mm/heap.h>
#include <core/thread.h>
#include <core/sync.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/init.h>
#include <drivers/video/framebuffer/framebuffer.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <lib/string.h>
#include <lib/math/math.h>

static bool framebuffer_initialized = false;
static bool rendering_enabled = false;
static uint8_t * backbuffer = NULL;
static uint64_t fb_size = 0;
static layer_t *layer_head = NULL;

static mutex_t framebuffer_mutex;
static spinlock_t layer_list_lock = SPINLOCK_INIT;
static spinlock_t state_lock = SPINLOCK_INIT;

static uint64_t target_fps = 60;
static double target_frame_time_ms = 0;
static uint64_t current_fps = 0;
double dt = 0; 

static inline uint32_t pack_color(color_t color) {
    return ((uint32_t)color.b) | ((uint32_t)color.g << 8) | ((uint32_t)color.r << 16) | ((uint32_t)color.a << 24);
}

static inline color_t unpack_color(uint32_t packed_color) {
    return (color_t){
        .b = (packed_color & 0xFF), 
        .g = ((packed_color >> 8) & 0xFF), 
        .r = ((packed_color >> 16) & 0xFF), 
        .a = ((packed_color >> 24) & 0xFF)
    };
}

static color_t fb_blend_color(color_t bg, color_t fg) {
    if (fg.a == 255) return fg;
    if (fg.a == 0)   return bg;

    uint32_t alpha = fg.a;
    uint32_t inv_alpha = 255 - alpha;

    color_t result;
    uint32_t r_sum = (fg.r * alpha) + (bg.r * inv_alpha) + 128;
    result.r = (uint8_t)((r_sum + (r_sum >> 8)) >> 8);

    uint32_t g_sum = (fg.g * alpha) + (bg.g * inv_alpha) + 128;
    result.g = (uint8_t)((g_sum + (g_sum >> 8)) >> 8);

    uint32_t b_sum = (fg.b * alpha) + (bg.b * inv_alpha) + 128;
    result.b = (uint8_t)((b_sum + (b_sum >> 8)) >> 8);

    result.a = 255;
    return result;
}

static inline void canvas_draw_hline(canvas_t *canvas, int64_t x1, int64_t x2, int64_t y, color_t color) {
    if (!canvas || !canvas->buffer || y < 0 || (uint64_t)y >= canvas->size.height) return;

    if (x1 > x2) { int64_t t = x1; x1 = x2; x2 = t; }
    if (x2 < 0 || x1 >= (int64_t)canvas->size.width) return;

    if (x1 < 0) x1 = 0;
    if (x2 >= (int64_t)canvas->size.width) x2 = (int64_t)canvas->size.width - 1;

    uint64_t stride = canvas->pitch / 4;
    uint32_t *row = canvas->buffer + (y * stride);
    int64_t count = x2 - x1 + 1;

    if (color.a == 255) {
        memset32(row + x1, pack_color(color), count);
    } else if (color.a > 0) {
        for (int64_t x = x1; x <= x2; x++) {
            color_t bg = unpack_color(row[x]);
            row[x] = pack_color(fb_blend_color(bg, color));
        }
    }
}

canvas_t *canvas_create(rect_size_t size) {
    canvas_t *canvas = (canvas_t *)kzalloc(sizeof(canvas_t));
    if (!canvas) return NULL;

    canvas->size = size;
    canvas->pitch = size.width * 4;
    canvas->buffer = (uint32_t *)kzalloc(size.height * canvas->pitch);

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
    uint64_t count = canvas->size.width * canvas->size.height;
    void *dest = canvas->buffer;
    memset32(dest, color_val, count);
}

void canvas_draw_pixel(canvas_t *canvas, point_t pos, color_t color) {
    if (!canvas || !canvas->buffer || 
        pos.x < 0 || (uint64_t)pos.x >= canvas->size.width || 
        pos.y < 0 || (uint64_t)pos.y >= canvas->size.height) {
        return;
    }
    uint64_t pixels_per_row = canvas->pitch / 4;
    uint64_t offset = (pos.y * pixels_per_row) + pos.x;
    color_t bg_color = unpack_color(*(uint32_t *)(canvas->buffer + offset));
    *(uint32_t *)(canvas->buffer + offset) = pack_color(fb_blend_color(bg_color, color));
}

void canvas_draw_triangle(canvas_t *canvas, point_t v1, point_t v2, point_t v3, color_t color, bool filled, uint64_t border_size) {
    if (!canvas) return;

    if (!filled) {
        canvas_draw_line(canvas, v1, v2, color, border_size);
        canvas_draw_line(canvas, v2, v3, color, border_size);
        canvas_draw_line(canvas, v3, v1, color, border_size);
        return;
    }

    if (v1.y > v2.y) { point_t t = v1; v1 = v2; v2 = t; }
    if (v1.y > v3.y) { point_t t = v1; v1 = v3; v3 = t; }
    if (v2.y > v3.y) { point_t t = v2; v2 = v3; v3 = t; }

    if (v1.y == v3.y) return;

    int64_t dy13 = v3.y - v1.y;
    int64_t dy12 = v2.y - v1.y;
    int64_t dy23 = v3.y - v2.y;

    int64_t dx13 = ((v3.x - v1.x) << 16) / dy13;
    int64_t dx12 = (dy12 != 0) ? (((v2.x - v1.x) << 16) / dy12) : 0;
    int64_t dx23 = (dy23 != 0) ? (((v3.x - v2.x) << 16) / dy23) : 0;

    int64_t cur_x1 = v1.x << 16;
    int64_t cur_x2 = v1.x << 16;

    for (int64_t y = v1.y; y < v2.y; y++) {
        canvas_draw_hline(canvas, cur_x1 >> 16, cur_x2 >> 16, y, color);
        cur_x1 += dx13;
        cur_x2 += dx12;
    }

    if (dy12 == 0) cur_x2 = v2.x << 16;
    for (int64_t y = v2.y; y <= v3.y; y++) {
        canvas_draw_hline(canvas, cur_x1 >> 16, cur_x2 >> 16, y, color);
        cur_x1 += dx13;
        cur_x2 += dx23;
    }
}

void canvas_draw_rectangle(canvas_t *canvas, point_t pos, rect_size_t size, color_t color, bool filled, uint64_t border_size) {
    if (!canvas || size.width == 0 || size.height == 0) return;

    if (filled) {
        for (uint64_t y = 0; y < size.height; y++) {
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)size.width - 1, pos.y + (int64_t)y, color);
        }
    } else {
        if (border_size == 0) return;
        uint64_t max_b = (border_size > size.height / 2) ? size.height / 2 : border_size;

        for (uint64_t b = 0; b < max_b; b++) {
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)size.width - 1, pos.y + (int64_t)b, color);
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)size.width - 1, pos.y + (int64_t)size.height - 1 - (int64_t)b, color);
        }
        for (int64_t y = pos.y + max_b; y <= pos.y + (int64_t)size.height - 1 - (int64_t)max_b; y++) {
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)max_b - 1, y, color);
            canvas_draw_hline(canvas, pos.x + (int64_t)size.width - (int64_t)max_b, pos.x + (int64_t)size.width - 1, y, color);
        }
    }
}

void canvas_draw_line(canvas_t *canvas, point_t start, point_t end, color_t color, uint64_t thickness) {
    if (!canvas || thickness == 0) return;

    int64_t dx = (end.x > start.x) ? (end.x - start.x) : (start.x - end.x);
    int64_t dy = (end.y > start.y) ? (end.y - start.y) : (start.y - end.y);
    int64_t sx = (start.x < end.x) ? 1 : -1;
    int64_t sy = (start.y < end.y) ? 1 : -1;
    int64_t err = dx - dy;

    int64_t r = (int64_t)(thickness / 2);

    while (1) {
        if (thickness == 1) {
            canvas_draw_pixel(canvas, start, color);
        } else {
            for (int64_t ty = -r; ty <= r; ty++) {
                canvas_draw_hline(canvas, start.x - r, start.x + r, start.y + ty, color);
            }
        }

        if (start.x == end.x && start.y == end.y) break;
        int64_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; start.x += sx; }
        if (e2 <  dx) { err += dx; start.y += sy; }
    }
}

void canvas_draw_circle(canvas_t *canvas, point_t center, uint64_t radius, color_t color, bool filled, uint64_t border_size) {
    if (!canvas || radius == 0) return;

    int64_t r_outer = (int64_t)radius;
    int64_t r_inner = (filled || border_size >= radius) ? 0 : (r_outer - (int64_t)border_size);

    int64_t r_outer_sq = r_outer * r_outer;
    int64_t r_inner_sq = r_inner * r_inner;

    for (int64_t y = -r_outer; y <= r_outer; y++) {
        int64_t y_sq = y * y;
        if (y_sq > r_outer_sq) continue;

        int64_t x_outer = (int64_t)sqrt(r_outer_sq - y_sq);

        if (y_sq < r_inner_sq) {
            int64_t x_inner = (int64_t)sqrt(r_inner_sq - y_sq);
            canvas_draw_hline(canvas, center.x - x_outer, center.x - x_inner, center.y + y, color);
            canvas_draw_hline(canvas, center.x + x_inner, center.x + x_outer, center.y + y, color);
        } else {
            canvas_draw_hline(canvas, center.x - x_outer, center.x + x_outer, center.y + y, color);
        }
    }
}

static void insert_layer_ordered(layer_t *layer) {
    uint64_t rflags = spinlock_acquire_irqsave(&layer_list_lock);
    layer_t **indirect = &layer_head;

    while (*indirect != NULL && (*indirect)->z_index <= layer->z_index) {
        indirect = &(*indirect)->next;
    }

    layer->next = *indirect;
    *indirect = layer;
    spinlock_release_irqrestore(&layer_list_lock, rflags);
}

static void remove_layer_ordered(layer_t *layer) {
    uint64_t rflags = spinlock_acquire_irqsave(&layer_list_lock);
    layer_t **indirect = &layer_head;

    while (*indirect != NULL && *indirect != layer) {
        indirect = &(*indirect)->next;
    }

    if (*indirect != NULL) {
        *indirect = layer->next;
        layer->next = NULL;
    }
    spinlock_release_irqrestore(&layer_list_lock, rflags);
}

layer_t *layer_create(rect_size_t size, int32_t z_index) {
    layer_t *layer = (layer_t *)kzalloc(sizeof(layer_t));
    if (!layer) return NULL;
    
    canvas_t *c = canvas_create(size);
    if (!c) {
        kfree((virt_addr_t)layer);
        return NULL;
    }

    layer->canvas = *c;
    kfree((virt_addr_t)c);
    layer->pos = (point_t){0, 0};
    layer->z_index = z_index;
    layer->visible = true;
    layer->next = NULL;
    mutex_init(&layer->lock, "layer_mutex");

    insert_layer_ordered(layer);
    return layer;
}

void layer_destroy(layer_t *layer) {
    if (!layer) return;
    remove_layer_ordered(layer);
    
    // Sicherstellen, dass das Layer nicht während eines aktiven Blits freigegeben wird
    mutex_lock(&layer->lock);
    if (layer->canvas.buffer) {
        kfree((virt_addr_t)(layer->canvas.buffer));
        layer->canvas.buffer = NULL;
    }
    mutex_unlock(&layer->lock);

    kfree((virt_addr_t)layer);
}

void layer_set_position(layer_t *layer, point_t pos) {
    if (!layer) return;
    mutex_lock(&layer->lock);
    layer->pos = pos;
    mutex_unlock(&layer->lock);
}

void layer_set_size(layer_t *layer, rect_size_t size) {
    if (!layer) return;
    mutex_lock(&layer->lock);
    
    // Reallokieren des Buffers mit Schutz
    uint32_t *new_buffer = (uint32_t *)kzalloc(size.height * size.width * 4);
    if (new_buffer) {
        if (layer->canvas.buffer) kfree((virt_addr_t)layer->canvas.buffer);
        layer->canvas.buffer = new_buffer;
        layer->canvas.size = size;
        layer->canvas.pitch = size.width * 4;
    }
    
    mutex_unlock(&layer->lock);
}

void layer_set_visible(layer_t *layer, bool visible) {
    if (!layer) return;
    mutex_lock(&layer->lock);
    layer->visible = visible;
    mutex_unlock(&layer->lock);
}

void layer_set_zindex(layer_t *layer, int32_t z_index) {
    if (!layer) return;
    remove_layer_ordered(layer);
    mutex_lock(&layer->lock);
    layer->z_index = z_index;
    mutex_unlock(&layer->lock);
    insert_layer_ordered(layer);
}

void layer_draw_begin(layer_t *layer) {
    if (layer) mutex_lock(&layer->lock);
}

void layer_draw_end(layer_t *layer) {
    if (layer) mutex_unlock(&layer->lock);
}

static void fb_composite_layer(layer_t *layer) {
    if (!layer) return;

    if (!mutex_trylock(&layer->lock)) return;

    if (!layer->visible || !layer->canvas.buffer) {
        mutex_unlock(&layer->lock);
        return;
    }

    canvas_t *layer_canvas = &layer->canvas;
    uint64_t screen_width = fb_get_width();
    uint64_t screen_height = fb_get_height();

    int64_t start_x = (layer->pos.x < 0) ? -layer->pos.x : 0;
    int64_t start_y = (layer->pos.y < 0) ? -layer->pos.y : 0;

    int64_t end_x = (layer->pos.x + (int64_t)layer_canvas->size.width > (int64_t)screen_width) ? (int64_t)screen_width - layer->pos.x : (int64_t)layer_canvas->size.width;
    int64_t end_y = (layer->pos.y + (int64_t)layer_canvas->size.height > (int64_t)screen_height) ? (int64_t)screen_height - layer->pos.y : (int64_t)layer_canvas->size.height;

    if (start_x < end_x && start_y < end_y) {
        for (int64_t ly = start_y; ly < end_y; ly++) {
            int64_t dest_y = layer->pos.y + ly;
            color_t *dest_row = (color_t *)(backbuffer + (dest_y * kernel_fb_info.fb_pitch));
            color_t *src_row  = (color_t *)((uint8_t *)layer_canvas->buffer + (ly * layer_canvas->pitch));

            for (int64_t lx = start_x; lx < end_x; lx++) {
                int64_t dest_x = layer->pos.x + lx;
                dest_row[dest_x] = fb_blend_color(dest_row[dest_x], src_row[lx]);
            }
        }
    }

    mutex_unlock(&layer->lock);
}

static void fb_compose() {
    uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
    bool enabled = rendering_enabled;
    spinlock_release_irqrestore(&state_lock, sflags);

    if (!enabled || !backbuffer) return;

    canvas_t backbuffer_canvas = {
        .buffer = (uint32_t *)backbuffer,
        .size = (rect_size_t){fb_get_width(), fb_get_height()},
        .pitch = kernel_fb_info.fb_pitch
    };
    canvas_clear(&backbuffer_canvas, COLOR_BLACK);

    uint64_t rflags = spinlock_acquire_irqsave(&layer_list_lock);
    layer_t *current = layer_head;

    while (current != NULL) {
        layer_t *next = current->next;
        spinlock_release_irqrestore(&layer_list_lock, rflags);

        fb_composite_layer(current);

        rflags = spinlock_acquire_irqsave(&layer_list_lock);
        current = next;
    }
    spinlock_release_irqrestore(&layer_list_lock, rflags);
}

static void fb_swap() {
    mutex_lock(&framebuffer_mutex);
    uint8_t *src = backbuffer;
    uint8_t *dest = (uint8_t *)kernel_fb_info.fb_addr;
    memcpy(dest, src, fb_size);
    mutex_unlock(&framebuffer_mutex);
}

void framebuffer_thread(void *arg) {
    (void)arg;

    uint64_t last_fps_update = hpet_uptime_ms();
    uint64_t frame_count = 0;
    uint64_t last_frame_start = hpet_uptime_ms();

    while (1) {
        uint64_t frame_start = hpet_uptime_ms();

        uint64_t frame_delta_ms = frame_start - last_frame_start;
        if (frame_delta_ms == 0) frame_delta_ms = 1;
        dt = (double)frame_delta_ms / 1000.0;
        last_frame_start = frame_start;

        uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
        bool initialized = framebuffer_initialized;
        double current_target_time = target_frame_time_ms;
        spinlock_release_irqrestore(&state_lock, sflags);

        if (!initialized) {
            thread_exit();
        }

        fb_compose();
        fb_swap();

        frame_count++;

        uint64_t current_time = hpet_uptime_ms();
        if (current_time - last_fps_update >= 1000) {
            sflags = spinlock_acquire_irqsave(&state_lock);
            current_fps = frame_count;
            spinlock_release_irqrestore(&state_lock, sflags);
            frame_count = 0;
            last_fps_update = current_time;
        }

        uint64_t frame_duration = hpet_uptime_ms() - frame_start;

        if (frame_duration < current_target_time) {
            while ((hpet_uptime_ms() - frame_start) < current_target_time) {
                __asm__ __volatile__("pause");
            }
        } else {
            thread_yield();
        }
    }
}

void fb_init() {
    uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
    target_frame_time_ms = (double)1000 / (double)target_fps;
    spinlock_release_irqrestore(&state_lock, sflags);

    fb_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
    serial_printf(COM1, "FB: Size is %d bytes\n", fb_size);

    mutex_init(&framebuffer_mutex, "fb_hardware_lock");

    backbuffer = (uint8_t *)kzalloc(fb_size);
    if (!backbuffer) {
        serial_printf(COM1, "FB: Failed to allocate backbuffer\n");
        return;
    }

    sflags = spinlock_acquire_irqsave(&state_lock);
    framebuffer_initialized = true;
    spinlock_release_irqrestore(&state_lock, sflags);

    thread_create(NULL, framebuffer_thread, NULL, "framebuffer_thread");
}

void fb_deinit() {
    uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
    framebuffer_initialized = false;
    spinlock_release_irqrestore(&state_lock, sflags);

    if (backbuffer) {
        kfree((virt_addr_t)backbuffer);
        backbuffer = NULL;
    }
}

void fb_enable_rendering() {
    uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
    rendering_enabled = true;
    spinlock_release_irqrestore(&state_lock, sflags);
}

void fb_disable_rendering() {
    uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
    rendering_enabled = false;
    spinlock_release_irqrestore(&state_lock, sflags);
}

void fb_set_target_fps(uint64_t fps) {
    if (fps == 0) return;
    uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
    target_fps = fps;
    target_frame_time_ms = (double)1000 / (double)fps;
    spinlock_release_irqrestore(&state_lock, sflags);
}

uint64_t fb_get_current_fps() {
    uint64_t sflags = spinlock_acquire_irqsave(&state_lock);
    uint64_t fps = current_fps;
    spinlock_release_irqrestore(&state_lock, sflags);
    return fps;
}