/**
 * @file compositor.h
 * @brief 2D software compositor/renderer
 * @author friedrichOsDev
 */

#pragma once

#include <kernel/sync.h>
#include <pch.h>

/** @brief Represents a color in packed 32-bit RGBA/BGRA format. */
typedef struct {
    uint8_t b; /**< Blue color channel (0-255). */
    uint8_t g; /**< Green color channel (0-255). */
    uint8_t r; /**< Red color channel (0-255). */
    uint8_t a; /**< Alpha channel (0 = fully transparent, 255 = fully opaque). */
} __attribute__((packed)) color_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

/**
 * @brief Constructs a color_t value with full opacity (alpha = 255).
 * @param r_val Red value (0-255).
 * @param g_val Green value (0-255).
 * @param b_val Blue value (0-255).
 */
#define MAKE_COLOR(r_val, g_val, b_val) \
    ((color_t){.b = (uint8_t)(b_val), .g = (uint8_t)(g_val), .r = (uint8_t)(r_val), .a = 255})

/**
 * @brief Constructs a color_t value with an explicit alpha value.
 * @param r_val Red value (0-255).
 * @param g_val Green value (0-255).
 * @param b_val Blue value (0-255).
 * @param a_val Alpha value (0-255).
 */
#define MAKE_COLOR_ALPHA(r_val, g_val, b_val, a_val) \
    ((color_t){.b = (uint8_t)(b_val), .g = (uint8_t)(g_val), .r = (uint8_t)(r_val), .a = (uint8_t)(a_val)})

/**
 * @brief Returns a copy of the specified color with a modified alpha value.
 * @param color Base color_t value.
 * @param a_val New alpha value (0-255).
 */
#define SET_COLOR_ALPHA(color, a_val) \
    ((color_t){.b = (color).b, .g = (color).g, .r = (color).r, .a = (uint8_t)(a_val)})

/* Predefined standard color definitions */
#define COLOR_BLACK MAKE_COLOR(0, 0, 0)
#define COLOR_DARK_GRAY MAKE_COLOR(64, 64, 64)
#define COLOR_GRAY MAKE_COLOR(128, 128, 128)
#define COLOR_LIGHT_GRAY MAKE_COLOR(211, 211, 211)
#define COLOR_WHITE MAKE_COLOR(255, 255, 255)
#define COLOR_RED MAKE_COLOR(255, 0, 0)
#define COLOR_GREEN MAKE_COLOR(0, 255, 0)
#define COLOR_BLUE MAKE_COLOR(0, 0, 255)
#define COLOR_YELLOW MAKE_COLOR(255, 255, 0)
#define COLOR_CYAN MAKE_COLOR(0, 255, 255)
#define COLOR_MAGENTA MAKE_COLOR(255, 0, 255)
#define COLOR_ORANGE MAKE_COLOR(255, 165, 0)
#define COLOR_PINK MAKE_COLOR(255, 192, 203)
#define COLOR_PURPLE MAKE_COLOR(128, 0, 128)
#define COLOR_BROWN MAKE_COLOR(139, 69, 19)
#define COLOR_TRANSPARENT MAKE_COLOR_ALPHA(0, 0, 0, 0)

#pragma GCC diagnostic pop

/** @brief Two-dimensional point / coordinate structure. */
typedef struct {
    int64_t x; /**< X coordinate. */
    int64_t y; /**< Y coordinate. */
} point_t;

/** @brief Rectangle dimension descriptor. */
typedef struct {
    uint64_t width;  /**< Width in pixels. */
    uint64_t height; /**< Height in pixels. */
} rect_size_t;

/** @brief Drawing canvas structure representing an in-memory pixel buffer. */
typedef struct {
    uint32_t *buffer; /**< Pointer to 32-bit pixel memory buffer. */
    rect_size_t size; /**< Dimensions of the canvas. */
    uint64_t pitch;   /**< Scanline pitch in bytes. */
} canvas_t;

/** @brief Represents a z-ordered rendering layer managed by the compositor. */
typedef struct layer {
    point_t pos;        /**< Layer position on screen. */
    int32_t z_index;    /**< Stacking order index (higher values drawn on top). */
    bool visible;       /**< Layer visibility toggle. */
    canvas_t canvas;    /**< Associated canvas buffer. */
    mutex_t lock;       /**< Mutex protecting layer rendering and updates. */
    struct layer *next; /**< Pointer to next layer in linked list. */
} layer_t;

kerr_t canvas_create(rect_size_t size, canvas_t **out);
kerr_t canvas_destroy(canvas_t *canvas);
kerr_t canvas_clear(canvas_t *canvas, color_t color);
kerr_t canvas_draw_pixel(canvas_t *canvas, point_t pos, color_t color);
kerr_t canvas_draw_text(canvas_t *canvas, point_t pos, const char *text, color_t color, uint32_t scale);
kerr_t canvas_draw_triangle(canvas_t *canvas, point_t vertex1, point_t vertex2, point_t vertex3, color_t color, bool filled, uint64_t border_size);
kerr_t canvas_draw_rectangle(canvas_t *canvas, point_t pos, rect_size_t size, color_t color, bool filled, uint64_t border_size);
kerr_t canvas_draw_line(canvas_t *canvas, point_t start, point_t end, color_t color, uint64_t thickness);
kerr_t canvas_draw_circle(canvas_t *canvas, point_t center, uint64_t radius, color_t color, bool filled, uint64_t border_size);

kerr_t layer_create(rect_size_t size, int32_t z_index, layer_t **out);
kerr_t layer_destroy(layer_t *layer);
kerr_t layer_set_position(layer_t *layer, point_t pos);
kerr_t layer_set_size(layer_t *layer, rect_size_t size);
kerr_t layer_set_visible(layer_t *layer, bool visible);
kerr_t layer_set_zindex(layer_t *layer, int32_t z_index);
kerr_t layer_draw_begin(layer_t *layer);
kerr_t layer_draw_end(layer_t *layer);

kerr_t compositor_init();
kerr_t compositor_deinit();
kerr_t compositor_enable_rendering();
kerr_t compositor_disable_rendering();
kerr_t compositor_set_target_fps(uint64_t fps);
uint64_t compositor_get_current_fps();
double compositor_get_delta_time();
uint64_t compositor_get_width();
uint64_t compositor_get_height();
rect_size_t compositor_get_size();
