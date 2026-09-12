/**
 * @file framebuffer.h
 * @brief Framebuffer driver with Canvas & Layer Rendering
 * @author friedrichOsDev
 */

#pragma once

#include <lib/math/math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAKE_COLOR(r_val, g_val, b_val) \
    ((color_t){.b = (b_val), .g = (g_val), .r = (r_val), .a = 255})

#define MAKE_COLOR_ALPHA(r_val, g_val, b_val, a_val) \
    ((color_t){.b = (b_val), .g = (g_val), .r = (r_val), .a = (a_val)})

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} __attribute__((packed)) color_t;

extern color_t black;
extern color_t gray;
extern color_t white;
extern color_t red;
extern color_t green;
extern color_t blue;

typedef struct {
    int64_t x1, y1;
    int64_t x2, y2;
    bool is_dirty;
} dirty_rect_t;

// Canvas structure representing a drawing target
typedef struct {
    uint32_t *buffer;
    uint64_t width;
    uint64_t height;
    uint64_t pitch; // in bytes
    dirty_rect_t dirty;
} canvas_t;

// Layer structure for scene composition
typedef struct layer {
    int64_t x;
    int64_t y;
    int32_t z_index;
    bool visible;
    canvas_t canvas;
    struct layer *next;
} layer_t;

// Canvas API
canvas_t *canvas_create(uint64_t width, uint64_t height);
void canvas_destroy(canvas_t *canvas);
void canvas_clear(canvas_t *canvas, color_t color);
void canvas_draw_pixel(canvas_t *canvas, uint64_t x, uint64_t y, color_t color);
void canvas_draw_rectangle(canvas_t *canvas, uint64_t x, uint64_t y, uint64_t width,
                           uint64_t height, color_t color, bool filled, uint64_t border_size);
void canvas_draw_line(canvas_t *canvas, int64_t x1, int64_t y1, int64_t x2, int64_t y2,
                      color_t color, uint64_t thickness);
void canvas_draw_circle(canvas_t *canvas, uint64_t xc, uint64_t yc, uint64_t radius,
                        color_t color, bool filled, uint64_t border_size);
void canvas_mark_dirty(canvas_t *canvas, int64_t x1, int64_t y1, int64_t x2, int64_t y2);

// Layer / Scene Manager API
layer_t *layer_create(uint64_t width, uint64_t height, int32_t z_index);
void layer_destroy(layer_t *layer);
void layer_set_position(layer_t *layer, int64_t x, int64_t y);
void layer_set_visible(layer_t *layer, bool visible);
void layer_set_zindex(layer_t *layer, int32_t z_index);

// Framebuffer Core Driver API
uint64_t fb_get_width(void);
uint64_t fb_get_height(void);
void fb_init(void);
void fb_render_scene(void);