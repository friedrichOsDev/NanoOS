/**
 * @file framebuffer.h
 * @brief Schnittstelle für den Framebuffer Treiber.
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <core/init.h>
#include <core/sync.h>

/**
 * @brief Definitionen einer Farbe in RGBA-Format.
 */
typedef struct {
    uint8_t b;  /**< Blau: 0 - 255 */
    uint8_t g;  /**< Grün: 0 - 255 */
    uint8_t r;  /**< Rot: 0 - 255 */
    uint8_t a;  /**< Alpha: 0 - 255 (0 = transparent, 255 = opaque) */
} __attribute__((packed)) color_t;

#define MAKE_COLOR(r_val, g_val, b_val)                                        \
    ((color_t){.b = (b_val), .g = (g_val), .r = (r_val), .a = 255})

#define MAKE_COLOR_ALPHA(r_val, g_val, b_val, a_val)                           \
    ((color_t){.b = (b_val), .g = (g_val), .r = (r_val), .a = (a_val)})

#define SET_COLOR_ALPHA(color, a_val)                                          \
    ((color_t){.b = (color).b, .g = (color).g, .r = (color).r, .a = (a_val)})

static const color_t COLOR_BLACK       = MAKE_COLOR(0, 0, 0);
static const color_t COLOR_DARK_GRAY   = MAKE_COLOR(64, 64, 64);
static const color_t COLOR_GRAY        = MAKE_COLOR(128, 128, 128);
static const color_t COLOR_LIGHT_GRAY  = MAKE_COLOR(211, 211, 211);
static const color_t COLOR_WHITE       = MAKE_COLOR(255, 255, 255);
static const color_t COLOR_RED         = MAKE_COLOR(255, 0, 0);
static const color_t COLOR_GREEN       = MAKE_COLOR(0, 255, 0);
static const color_t COLOR_BLUE        = MAKE_COLOR(0, 0, 255);
static const color_t COLOR_YELLOW      = MAKE_COLOR(255, 255, 0);
static const color_t COLOR_CYAN        = MAKE_COLOR(0, 255, 255);
static const color_t COLOR_MAGENTA     = MAKE_COLOR(255, 0, 255);
static const color_t COLOR_ORANGE      = MAKE_COLOR(255, 165, 0);
static const color_t COLOR_PINK        = MAKE_COLOR(255, 192, 203);
static const color_t COLOR_PURPLE      = MAKE_COLOR(128, 0, 128);
static const color_t COLOR_BROWN       = MAKE_COLOR(139, 69, 19);
static const color_t COLOR_TRANSPARENT = MAKE_COLOR_ALPHA(0, 0, 0, 0);

typedef struct {
    int64_t x;
    int64_t y;
} point_t;

typedef struct {
    uint64_t width;
    uint64_t height;
} rect_size_t;

typedef struct {
    uint32_t *buffer;
    rect_size_t size;
    uint64_t pitch;
} canvas_t;

typedef struct layer {
    point_t pos;
    int32_t z_index;
    bool visible;
    canvas_t canvas;
    mutex_t lock;
    struct layer *next;
} layer_t;

canvas_t *canvas_create(rect_size_t size);
void canvas_destroy(canvas_t *canvas);
void canvas_clear(canvas_t *canvas, color_t color);
void canvas_draw_pixel(canvas_t *canvas, point_t pos, color_t color);
void canvas_draw_triangle(canvas_t *canvas, point_t vertex1, point_t vertex2, point_t vertex3, color_t color, bool filled, uint64_t border_size);
void canvas_draw_rectangle(canvas_t *canvas, point_t pos, rect_size_t size, color_t color, bool filled, uint64_t border_size);
void canvas_draw_line(canvas_t *canvas, point_t start, point_t end, color_t color, uint64_t thickness);
void canvas_draw_circle(canvas_t *canvas, point_t center, uint64_t radius, color_t color, bool filled, uint64_t border_size);

layer_t *layer_create(rect_size_t size, int32_t z_index);
void layer_destroy(layer_t *layer);
void layer_set_position(layer_t *layer, point_t pos);
void layer_set_size(layer_t *layer, rect_size_t size);
void layer_set_visible(layer_t *layer, bool visible);
void layer_set_zindex(layer_t *layer, int32_t z_index);
void layer_draw_begin(layer_t *layer);
void layer_draw_end(layer_t *layer);

void fb_init();
void fb_deinit();
void fb_enable_rendering();
void fb_disable_rendering();
void fb_set_target_fps(uint64_t fps);
uint64_t fb_get_current_fps();

static inline uint64_t fb_get_width(void) {
    return kernel_fb_info.fb_width;
}

static inline uint64_t fb_get_height(void) {
    return kernel_fb_info.fb_height;
}

static inline rect_size_t fb_get_size(void) {
    return (rect_size_t){.width = fb_get_width(), .height = fb_get_height()};
}