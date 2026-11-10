/**
 * @file compositor.c
 * @brief 2D software compositor/renderer
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <drivers/gfx/compositor.h>
#include <drivers/gfx/gfx.h>
#include <kernel/sync.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <pch.h>
#include <sched/sched.h>
#include <sched/thread.h>

static bool compositor_initialized = false;
static bool rendering_enabled = false;
static uint8_t *backbuffer = NULL;
static size_t fb_size = 0;
static video_mode_t active_mode = {0};
static layer_t *layer_head = NULL;

static mutex_t compositor_mutex;
static spinlock_t layer_list_lock = SPINLOCK_INIT;
static spinlock_t state_lock = SPINLOCK_INIT;

static uint64_t target_fps = 60;
static double target_frame_time_ms = 0.0;
static uint64_t current_fps = 0;
static double delta_time = 0.0;

/**
 * @brief Packs a color_t structure into a 32-bit integer.
 *
 * @param[in] color Source color.
 * @return Packed 32-bit RGBA value.
 */
static inline uint32_t pack_color(color_t color) {
    return ((uint32_t)color.b) |
           ((uint32_t)color.g << 8) |
           ((uint32_t)color.r << 16) |
           ((uint32_t)color.a << 24);
}

/**
 * @brief Unpacks a 32-bit packed RGBA value into a color_t structure.
 *
 * @param[in] packed_color 32-bit integer color.
 * @return Unpacked color_t structure.
 */
static inline color_t unpack_color(uint32_t packed_color) {
    return (color_t){
        .b = (uint8_t)(packed_color & 0xFF),
        .g = (uint8_t)((packed_color >> 8) & 0xFF),
        .r = (uint8_t)((packed_color >> 16) & 0xFF),
        .a = (uint8_t)((packed_color >> 24) & 0xFF)};
}

/**
 * @brief Performs alpha blending between foreground and background colors.
 *
 * @param[in] bg Background color.
 * @param[in] fg Foreground color with alpha channel.
 * @return Blended color value.
 */
static inline color_t blend_color(color_t bg, color_t fg) {
    if (fg.a == 255) {
        return fg;
    }
    if (fg.a == 0) {
        return bg;
    }

    uint32_t alpha = fg.a;
    uint32_t inv_alpha = 255 - alpha;

    color_t result;
    uint32_t r_sum = ((uint32_t)fg.r * alpha) + ((uint32_t)bg.r * inv_alpha) + 128;
    result.r = (uint8_t)((r_sum + (r_sum >> 8)) >> 8);

    uint32_t g_sum = ((uint32_t)fg.g * alpha) + ((uint32_t)bg.g * inv_alpha) + 128;
    result.g = (uint8_t)((g_sum + (g_sum >> 8)) >> 8);

    uint32_t b_sum = ((uint32_t)fg.b * alpha) + ((uint32_t)bg.b * inv_alpha) + 128;
    result.b = (uint8_t)((b_sum + (b_sum >> 8)) >> 8);

    result.a = 255;
    return result;
}

/**
 * @brief Draws a horizontal span of pixels with clipping and alpha blending.
 *
 * @param[in,out] canvas Target canvas.
 * @param[in] x1 Starting X coordinate.
 * @param[in] x2 Ending X coordinate.
 * @param[in] y Y scanline coordinate.
 * @param[in] color Drawing color.
 */
static inline void canvas_draw_hline(canvas_t *canvas, int64_t x1, int64_t x2, int64_t y, color_t color) {
    if (!canvas || !canvas->buffer || y < 0 || (uint64_t)y >= canvas->size.height) {
        return;
    }

    if (x1 > x2) {
        int64_t t = x1;
        x1 = x2;
        x2 = t;
    }
    if (x2 < 0 || x1 >= (int64_t)canvas->size.width) {
        return;
    }

    if (x1 < 0) {
        x1 = 0;
    }
    if (x2 >= (int64_t)canvas->size.width) {
        x2 = (int64_t)canvas->size.width - 1;
    }

    size_t stride = (size_t)(canvas->pitch / sizeof(uint32_t));
    uint32_t *row = canvas->buffer + ((size_t)y * stride);
    size_t count = (size_t)(x2 - x1 + 1);

    if (color.a == 255) {
        memset32(row + (size_t)x1, pack_color(color), count);
    } else if (color.a > 0) {
        for (int64_t x = x1; x <= x2; x++) {
            size_t idx = (size_t)x;
            color_t bg = unpack_color(row[idx]);
            row[idx] = pack_color(blend_color(bg, color));
        }
    }
}

/**
 * @brief Allocates and initializes a new canvas with the specified size.
 *
 * @param[in] size Dimensions of the canvas.
 * @param[out] out Double pointer to store the created canvas pointer.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_create(rect_size_t size, canvas_t **out) {
    if (!out || size.width == 0 || size.height == 0) {
        return E_INVAL;
    }

    virt_addr_t mem = 0;
    K_TRY(kzalloc(sizeof(canvas_t), &mem));
    canvas_t *canvas = (canvas_t *)mem;

    canvas->size = size;
    canvas->pitch = size.width * sizeof(uint32_t);

    virt_addr_t buf_mem = 0;
    size_t buffer_size = (size_t)(size.height * canvas->pitch);
    kerr_t err = kzalloc(buffer_size, &buf_mem);
    if (k_failed(err)) {
        kfree((virt_addr_t)canvas);
        return err;
    }

    canvas->buffer = (uint32_t *)buf_mem;
    *out = canvas;
    return E_OK;
}

/**
 * @brief Destroys a canvas and frees its allocated memory.
 *
 * @param[in,out] canvas Pointer to the canvas to destroy.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_destroy(canvas_t *canvas) {
    if (!canvas) {
        return E_INVAL;
    }

    if (canvas->buffer) {
        kfree((virt_addr_t)canvas->buffer);
        canvas->buffer = NULL;
    }

    kfree((virt_addr_t)canvas);
    return E_OK;
}

/**
 * @brief Clears the entire canvas using the specified color.
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] color Fill color.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_clear(canvas_t *canvas, color_t color) {
    if (!canvas || !canvas->buffer) {
        return E_INVAL;
    }

    uint32_t color_val = pack_color(color);
    size_t count = (size_t)(canvas->size.width * canvas->size.height);
    memset32(canvas->buffer, color_val, count);
    return E_OK;
}

/**
 * @brief Draws a single pixel onto the canvas with alpha blending.
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] pos Coordinate position.
 * @param[in] color Pixel color.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_draw_pixel(canvas_t *canvas, point_t pos, color_t color) {
    if (!canvas || !canvas->buffer ||
        pos.x < 0 || (uint64_t)pos.x >= canvas->size.width ||
        pos.y < 0 || (uint64_t)pos.y >= canvas->size.height) {
        return E_INVAL;
    }

    size_t pixels_per_row = (size_t)(canvas->pitch / sizeof(uint32_t));
    size_t offset = ((size_t)pos.y * pixels_per_row) + (size_t)pos.x;
    color_t bg_color = unpack_color(canvas->buffer[offset]);
    canvas->buffer[offset] = pack_color(blend_color(bg_color, color));
    return E_OK;
}

/* Embedded PSF2 font, linked from assets/font.psf. */
extern const uint8_t nanoos_font_start[];
extern const uint8_t nanoos_font_end[];

typedef struct {
    uint32_t header_size;
    uint32_t flags;
    uint32_t glyph_count;
    uint32_t glyph_size;
    uint32_t height;
    uint32_t width;
    const uint8_t *glyph_data;
    const uint8_t *unicode_table;
    const uint8_t *end;
} psf2_font_t;

static psf2_font_t builtin_font;
static bool builtin_font_checked = false;
static bool builtin_font_valid = false;

static uint32_t font_read_u32(const uint8_t *ptr) {
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

static bool font_init(void) {
    if (builtin_font_checked) {
        return builtin_font_valid;
    }
    builtin_font_checked = true;

    const uint8_t *start = nanoos_font_start;
    const uint8_t *end = nanoos_font_end;
    if (end <= start || (size_t)(end - start) < 32U) {
        return false;
    }
    if (font_read_u32(start) != UINT32_C(0x864AB572)) {
        return false;
    }

    uint32_t header_size = font_read_u32(start + 8);
    uint32_t flags = font_read_u32(start + 12);
    uint32_t glyph_count = font_read_u32(start + 16);
    uint32_t glyph_size = font_read_u32(start + 20);
    uint32_t height = font_read_u32(start + 24);
    uint32_t width = font_read_u32(start + 28);

    if (header_size < 32U || header_size > (size_t)(end - start) ||
        glyph_count == 0U || glyph_size == 0U ||
        width == 0U || width > 32U || height == 0U || height > 64U) {
        return false;
    }

    size_t bitmap_row_size = ((size_t)width + 7U) / 8U;
    if (bitmap_row_size > SIZE_MAX / (size_t)height ||
        bitmap_row_size * (size_t)height > glyph_size ||
        (size_t)glyph_count > (SIZE_MAX - (size_t)header_size) / (size_t)glyph_size) {
        return false;
    }

    size_t glyph_bytes = (size_t)glyph_count * (size_t)glyph_size;
    size_t table_offset = (size_t)header_size + glyph_bytes;
    if (table_offset > (size_t)(end - start)) {
        return false;
    }

    builtin_font = (psf2_font_t){
        .header_size = header_size,
        .flags = flags,
        .glyph_count = glyph_count,
        .glyph_size = glyph_size,
        .height = height,
        .width = width,
        .glyph_data = start + header_size,
        .unicode_table = start + table_offset,
        .end = end
    };

    if ((flags & 1U) != 0U && builtin_font.unicode_table >= end) {
        return false;
    }

    builtin_font_valid = true;
    return true;
}

/* Decode one UTF-8 code point. Invalid input consumes one byte and yields U+FFFD. */
static uint32_t font_utf8_decode(const uint8_t *text, size_t *consumed) {
    uint8_t first = text[0];
    *consumed = 1U;
    if (first < 0x80U) {
        return first;
    }

    uint32_t value;
    size_t count;
    if (first >= 0xC2U && first <= 0xDFU) {
        value = (uint32_t)(first & 0x1FU);
        count = 2U;
    } else if (first >= 0xE0U && first <= 0xEFU) {
        value = (uint32_t)(first & 0x0FU);
        count = 3U;
    } else if (first >= 0xF0U && first <= 0xF4U) {
        value = (uint32_t)(first & 0x07U);
        count = 4U;
    } else {
        return UINT32_C(0xFFFD);
    }

    if (remaining < count) {
        return UINT32_C(0xFFFD);
    }
    for (size_t i = 1U; i < count; i++) {
        if ((text[i] & 0xC0U) != 0x80U) {
            return UINT32_C(0xFFFD);
        }
        value = (value << 6) | (uint32_t)(text[i] & 0x3FU);
    }

    if ((count == 3U && value < UINT32_C(0x800)) ||
        (count == 4U && value < UINT32_C(0x10000)) ||
        value > UINT32_C(0x10FFFF) ||
        (value >= UINT32_C(0xD800) && value <= UINT32_C(0xDFFF))) {
        return UINT32_C(0xFFFD);
    }
    *consumed = count;
    return value;
}

static uint32_t font_find_glyph(uint32_t codepoint) {
    if ((builtin_font.flags & 1U) == 0U) {
        return codepoint < builtin_font.glyph_count ? codepoint : 0U;
    }

    const uint8_t *cursor = builtin_font.unicode_table;
    const uint8_t *end = builtin_font.end;
    uint32_t fallback = 0U;

    for (uint32_t glyph = 0U; glyph < builtin_font.glyph_count && cursor < end; glyph++) {
        bool sequence = false;
        while (cursor < end && *cursor != 0xFFU) {
            if (*cursor == 0xFEU) {
                sequence = true;
                cursor++;
                continue;
            }
            size_t consumed = 0U;
            uint32_t current = font_utf8_decode(cursor, &consumed);
            if (consumed == 0U) {
                break;
            }
            cursor += consumed;
            if (!sequence && current == codepoint) {
                return glyph;
            }
            if (!sequence && current == (uint32_t)'?') {
                fallback = glyph;
            }
        }
        if (cursor < end && *cursor == 0xFFU) {
            cursor++;
        }
    }
    return fallback;
}

/**
 * @brief Draws a UTF-8 string using the embedded PSF2 bitmap font.
 *
 * @param[in,out] canvas Target canvas.
 * @param[in] pos Top-left position.
 * @param[in] text Null-terminated UTF-8 string.
 * @param[in] color Foreground color.
 * @param[in] scale Integer scale from 1 to 16.
 * @return E_OK on success, or an error code.
 */
kerr_t canvas_draw_text(canvas_t *canvas, point_t pos, const char *text, color_t color, uint32_t scale) {
    if (!canvas || !canvas->buffer || !text || scale == 0U || scale > 16U ||
        pos.x < 0 || pos.y < 0) {
        return E_INVAL;
    }
    if (!font_init()) {
        return E_NOINIT;
    }
    if (color.a == 0U) {
        return E_OK;
    }

    int64_t origin_x = pos.x;
    int64_t cursor_x = pos.x;
    int64_t cursor_y = pos.y;
    int64_t advance_x = (int64_t)(builtin_font.width * scale);
    int64_t advance_y = (int64_t)(builtin_font.height * scale);
    int64_t tab_width = advance_x * 4;

    const uint8_t *input = (const uint8_t *)text;
    while (*input != 0U) {
        size_t remaining = 0U;
        while (input[remaining] != 0U) {
            remaining++;
        }
        size_t consumed = 0U;
        uint32_t codepoint = font_utf8_decode(input, remaining, &consumed);
        input += consumed;

        if (codepoint == (uint32_t)'\n') {
            cursor_x = origin_x;
            if (cursor_y > INT64_MAX - advance_y) {
                break;
            }
            cursor_y += advance_y;
            continue;
        }
        if (codepoint == (uint32_t)'\r') {
            continue;
        }
        if (codepoint == (uint32_t)'\t') {
            int64_t relative = cursor_x - origin_x;
            cursor_x = origin_x + ((relative / tab_width) + 1) * tab_width;
            continue;
        }

        if (cursor_x > INT64_MAX - advance_x || cursor_y > INT64_MAX - advance_y) {
            break;
        }

        uint32_t glyph = font_find_glyph(codepoint);
        const uint8_t *bitmap = builtin_font.glyph_data + ((size_t)glyph * builtin_font.glyph_size);
        size_t bytes_per_row = ((size_t)builtin_font.width + 7U) / 8U;

        for (uint32_t gy = 0U; gy < builtin_font.height; gy++) {
            for (uint32_t gx = 0U; gx < builtin_font.width; gx++) {
                size_t byte_index = ((size_t)gy * bytes_per_row) + ((size_t)gx / 8U);
                uint8_t mask = (uint8_t)(0x80U >> (gx % 8U));
                if ((bitmap[byte_index] & mask) == 0U) {
                    continue;
                }

                int64_t pixel_x = cursor_x + (int64_t)(gx * scale);
                int64_t pixel_y = cursor_y + (int64_t)(gy * scale);
                for (uint32_t sy = 0U; sy < scale; sy++) {
                    int64_t y = pixel_y + (int64_t)sy;
                    if (y < 0 || (uint64_t)y >= canvas->size.height) {
                        continue;
                    }
                    size_t stride = (size_t)(canvas->pitch / sizeof(uint32_t));
                    uint32_t *row = canvas->buffer + ((size_t)y * stride);
                    for (uint32_t sx = 0U; sx < scale; sx++) {
                        int64_t px = pixel_x + (int64_t)sx;
                        if (px < 0 || (uint64_t)px >= canvas->size.width) {
                            continue;
                        }
                        size_t offset = (size_t)px;
                        color_t background = unpack_color(row[offset]);
                        row[offset] = pack_color(blend_color(background, color));
                    }
                }
            }
        }
        cursor_x += advance_x;
    }
    return E_OK;
}

/**
 * @brief Draws a triangle onto the canvas (either outline or filled).
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] v1 First triangle vertex coordinate.
 * @param[in] v2 Second triangle vertex coordinate.
 * @param[in] v3 Third triangle vertex coordinate.
 * @param[in] color Drawing color.
 * @param[in] filled True to rasterize a filled triangle, false for wireframe outline.
 * @param[in] border_size Outline thickness when filled is false.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_draw_triangle(canvas_t *canvas, point_t v1, point_t v2, point_t v3, color_t color, bool filled, uint64_t border_size) {
    if (!canvas || !canvas->buffer) {
        return E_INVAL;
    }

    if (!filled) {
        K_TRY(canvas_draw_line(canvas, v1, v2, color, border_size));
        K_TRY(canvas_draw_line(canvas, v2, v3, color, border_size));
        K_TRY(canvas_draw_line(canvas, v3, v1, color, border_size));
        return E_OK;
    }

    /* Sort vertices by Y coordinate */
    if (v1.y > v2.y) {
        point_t t = v1;
        v1 = v2;
        v2 = t;
    }
    if (v1.y > v3.y) {
        point_t t = v1;
        v1 = v3;
        v3 = t;
    }
    if (v2.y > v3.y) {
        point_t t = v2;
        v2 = v3;
        v3 = t;
    }

    if (v1.y == v3.y) {
        return E_OK;
    }

    int64_t dy13 = v3.y - v1.y;
    int64_t dy12 = v2.y - v1.y;
    int64_t dy23 = v3.y - v2.y;

    int64_t dx13 = ((v3.x - v1.x) << 16) / dy13;
    int64_t dx12 = (dy12 != 0) ? (((v2.x - v1.x) << 16) / dy12) : 0;
    int64_t dx23 = (dy23 != 0) ? (((v3.x - v2.x) << 16) / dy23) : 0;

    int64_t cur_x1 = v1.x << 16;
    int64_t cur_x2 = v1.x << 16;

    /* Rasterize top sub-triangle */
    for (int64_t y = v1.y; y < v2.y; y++) {
        canvas_draw_hline(canvas, cur_x1 >> 16, cur_x2 >> 16, y, color);
        cur_x1 += dx13;
        cur_x2 += dx12;
    }

    /* Rasterize bottom sub-triangle */
    if (dy12 == 0) {
        cur_x2 = v2.x << 16;
    }
    for (int64_t y = v2.y; y <= v3.y; y++) {
        canvas_draw_hline(canvas, cur_x1 >> 16, cur_x2 >> 16, y, color);
        cur_x1 += dx13;
        cur_x2 += dx23;
    }

    return E_OK;
}

/**
 * @brief Draws a rectangle onto the canvas.
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] pos Top-left position of rectangle.
 * @param[in] size Dimensions of rectangle.
 * @param[in] color Drawing color.
 * @param[in] filled True to rasterize a filled rectangle, false for border outline.
 * @param[in] border_size Border thickness when filled is false.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_draw_rectangle(canvas_t *canvas, point_t pos, rect_size_t size, color_t color, bool filled, uint64_t border_size) {
    if (!canvas || !canvas->buffer || size.width == 0 || size.height == 0) {
        return E_INVAL;
    }

    if (filled) {
        for (uint64_t y = 0; y < size.height; y++) {
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)size.width - 1, pos.y + (int64_t)y, color);
        }
    } else {
        if (border_size == 0) {
            return E_OK;
        }
        uint64_t max_b = (border_size > size.height / 2) ? (size.height / 2) : border_size;

        /* Top and bottom edges */
        for (uint64_t b = 0; b < max_b; b++) {
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)size.width - 1, pos.y + (int64_t)b, color);
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)size.width - 1, pos.y + (int64_t)size.height - 1 - (int64_t)b, color);
        }
        /* Left and right edges */
        int64_t start_y = pos.y + (int64_t)max_b;
        int64_t end_y = pos.y + (int64_t)size.height - 1 - (int64_t)max_b;
        for (int64_t y = start_y; y <= end_y; y++) {
            canvas_draw_hline(canvas, pos.x, pos.x + (int64_t)max_b - 1, y, color);
            canvas_draw_hline(canvas, pos.x + (int64_t)size.width - (int64_t)max_b, pos.x + (int64_t)size.width - 1, y, color);
        }
    }
    return E_OK;
}

/**
 * @brief Draws a line between two points using Bresenham's line algorithm.
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] start Starting point coordinate.
 * @param[in] end Ending point coordinate.
 * @param[in] color Drawing color.
 * @param[in] thickness Thickness of the line in pixels.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_draw_line(canvas_t *canvas, point_t start, point_t end, color_t color, uint64_t thickness) {
    if (!canvas || !canvas->buffer || thickness == 0) {
        return E_INVAL;
    }

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

        if (start.x == end.x && start.y == end.y) {
            break;
        }
        int64_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            start.x += sx;
        }
        if (e2 < dx) {
            err += dx;
            start.y += sy;
        }
    }
    return E_OK;
}

/**
 * @brief Draws a circle centered at a specific point.
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] center Center coordinate of circle.
 * @param[in] radius Radius in pixels.
 * @param[in] color Drawing color.
 * @param[in] filled True to rasterize a filled circle, false for border outline.
 * @param[in] border_size Border thickness when filled is false.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_draw_circle(canvas_t *canvas, point_t center, uint64_t radius, color_t color, bool filled, uint64_t border_size) {
    if (!canvas || !canvas->buffer || radius == 0) {
        return E_INVAL;
    }

    int64_t r_outer = (int64_t)radius;
    int64_t r_inner = (filled || border_size >= radius) ? 0 : (r_outer - (int64_t)border_size);

    int64_t r_outer_sq = r_outer * r_outer;
    int64_t r_inner_sq = r_inner * r_inner;

    for (int64_t y = -r_outer; y <= r_outer; y++) {
        int64_t y_sq = y * y;
        if (y_sq > r_outer_sq) {
            continue;
        }

        int64_t x_outer = (int64_t)sqrt((double)(r_outer_sq - y_sq));

        if (y_sq < r_inner_sq) {
            int64_t x_inner = (int64_t)sqrt((double)(r_inner_sq - y_sq));
            canvas_draw_hline(canvas, center.x - x_outer, center.x - x_inner, center.y + y, color);
            canvas_draw_hline(canvas, center.x + x_inner, center.x + x_outer, center.y + y, color);
        } else {
            canvas_draw_hline(canvas, center.x - x_outer, center.x + x_outer, center.y + y, color);
        }
    }
    return E_OK;
}

/**
 * @brief Inserts a layer into the ordered layer list sorted ascendingly by Z-index.
 *
 * @param[in,out] layer Pointer to layer to insert.
 */
static void insert_layer_ordered(layer_t *layer) {
    irq_flags_t rflags = spinlock_acquire_irqsave(&layer_list_lock);
    layer_t **indirect = &layer_head;

    while (*indirect != NULL && (*indirect)->z_index <= layer->z_index) {
        indirect = &(*indirect)->next;
    }

    layer->next = *indirect;
    *indirect = layer;
    spinlock_release_irqrestore(&layer_list_lock, rflags);
}

/**
 * @brief Safely removes a layer from the global ordered layer list.
 *
 * @param[in,out] layer Pointer to layer to remove.
 */
static void remove_layer_ordered(layer_t *layer) {
    irq_flags_t rflags = spinlock_acquire_irqsave(&layer_list_lock);
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

/**
 * @brief Creates and registers a new layer in the compositor.
 *
 * @param[in] size Dimensions of the layer canvas.
 * @param[in] z_index Stacking index for drawing order.
 * @param[out] out Double pointer to store the created layer pointer.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_create(rect_size_t size, int32_t z_index, layer_t **out) {
    if (!out || size.width == 0 || size.height == 0) {
        return E_INVAL;
    }

    virt_addr_t mem = 0;
    K_TRY(kzalloc(sizeof(layer_t), &mem));
    layer_t *layer = (layer_t *)mem;

    canvas_t *c = NULL;
    kerr_t err = canvas_create(size, &c);
    if (k_failed(err)) {
        kfree((virt_addr_t)layer);
        return err;
    }

    layer->canvas = *c;
    kfree((virt_addr_t)c);

    layer->pos = (point_t){0, 0};
    layer->z_index = z_index;
    layer->visible = true;
    layer->next = NULL;
    mutex_init(&layer->lock, "layer_mutex");

    insert_layer_ordered(layer);
    *out = layer;
    return E_OK;
}

/**
 * @brief Destroys a layer, unregisters it from the compositor, and frees its resources.
 *
 * @param[in,out] layer Pointer to layer to destroy.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_destroy(layer_t *layer) {
    if (!layer) {
        return E_INVAL;
    }

    remove_layer_ordered(layer);

    mutex_lock(&layer->lock);
    if (layer->canvas.buffer) {
        kfree((virt_addr_t)layer->canvas.buffer);
        layer->canvas.buffer = NULL;
    }
    mutex_unlock(&layer->lock);

    kfree((virt_addr_t)layer);
    return E_OK;
}

/**
 * @brief Sets the on-screen position of a layer.
 *
 * @param[in,out] layer Pointer to layer.
 * @param[in] pos New top-left screen position.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_set_position(layer_t *layer, point_t pos) {
    if (!layer) {
        return E_INVAL;
    }
    mutex_lock(&layer->lock);
    layer->pos = pos;
    mutex_unlock(&layer->lock);
    return E_OK;
}

/**
 * @brief Resizes a layer and reallocates its backing canvas buffer.
 *
 * @param[in,out] layer Pointer to layer.
 * @param[in] size New dimensions.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_set_size(layer_t *layer, rect_size_t size) {
    if (!layer || size.width == 0 || size.height == 0) {
        return E_INVAL;
    }

    size_t new_size = (size_t)(size.height * size.width * sizeof(uint32_t));
    virt_addr_t mem = 0;
    K_TRY(kzalloc(new_size, &mem));

    mutex_lock(&layer->lock);
    if (layer->canvas.buffer) {
        kfree((virt_addr_t)layer->canvas.buffer);
    }
    layer->canvas.buffer = (uint32_t *)mem;
    layer->canvas.size = size;
    layer->canvas.pitch = size.width * sizeof(uint32_t);
    mutex_unlock(&layer->lock);

    return E_OK;
}

/**
 * @brief Sets whether a layer is visible.
 *
 * @param[in,out] layer Pointer to layer.
 * @param[in] visible Visibility flag.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_set_visible(layer_t *layer, bool visible) {
    if (!layer) {
        return E_INVAL;
    }
    mutex_lock(&layer->lock);
    layer->visible = visible;
    mutex_unlock(&layer->lock);
    return E_OK;
}

/**
 * @brief Updates a layer's Z-index and reorders it within the layer stack.
 *
 * @param[in,out] layer Pointer to layer.
 * @param[in] z_index New stacking index.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_set_zindex(layer_t *layer, int32_t z_index) {
    if (!layer) {
        return E_INVAL;
    }
    remove_layer_ordered(layer);
    mutex_lock(&layer->lock);
    layer->z_index = z_index;
    mutex_unlock(&layer->lock);
    insert_layer_ordered(layer);
    return E_OK;
}

/**
 * @brief Acquires the layer lock prior to batch drawing operations.
 *
 * @param[in,out] layer Pointer to layer.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_draw_begin(layer_t *layer) {
    if (!layer) {
        return E_INVAL;
    }
    mutex_lock(&layer->lock);
    return E_OK;
}

/**
 * @brief Releases the layer lock after batch drawing operations.
 *
 * @param[in,out] layer Pointer to layer.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_draw_end(layer_t *layer) {
    if (!layer) {
        return E_INVAL;
    }
    mutex_unlock(&layer->lock);
    return E_OK;
}

/**
 * @brief Blits a single layer into the compositor backbuffer with alpha blending.
 *
 * @param[in,out] layer Pointer to layer to composite.
 */
static void composite_layer(layer_t *layer) {
    if (!layer) {
        return;
    }

    if (!mutex_trylock(&layer->lock)) {
        return;
    }

    if (!layer->visible || !layer->canvas.buffer) {
        mutex_unlock(&layer->lock);
        return;
    }

    canvas_t *layer_canvas = &layer->canvas;
    uint64_t screen_width = active_mode.width;
    uint64_t screen_height = active_mode.height;

    int64_t start_x = (layer->pos.x < 0) ? -layer->pos.x : 0;
    int64_t start_y = (layer->pos.y < 0) ? -layer->pos.y : 0;

    int64_t end_x = (layer->pos.x + (int64_t)layer_canvas->size.width > (int64_t)screen_width) ? ((int64_t)screen_width - layer->pos.x) : (int64_t)layer_canvas->size.width;
    int64_t end_y = (layer->pos.y + (int64_t)layer_canvas->size.height > (int64_t)screen_height) ? ((int64_t)screen_height - layer->pos.y) : (int64_t)layer_canvas->size.height;

    if (start_x < end_x && start_y < end_y) {
        for (int64_t ly = start_y; ly < end_y; ly++) {
            int64_t dest_y = layer->pos.y + ly;
            size_t dest_stride = (size_t)(dest_y * (int64_t)active_mode.pitch);
            color_t *dest_row = (color_t *)(backbuffer + dest_stride);

            size_t src_stride = (size_t)(ly * (int64_t)layer_canvas->pitch);
            color_t *src_row = (color_t *)((uint8_t *)layer_canvas->buffer + src_stride);

            for (int64_t lx = start_x; lx < end_x; lx++) {
                int64_t dest_x = layer->pos.x + lx;
                dest_row[dest_x] = blend_color(dest_row[dest_x], src_row[lx]);
            }
        }
    }

    mutex_unlock(&layer->lock);
}

/** @brief Composites all visible layers according to their Z-index into the backbuffer. */
static void compositor_compose() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    bool enabled = rendering_enabled;
    spinlock_release_irqrestore(&state_lock, sflags);

    if (!enabled || !backbuffer) {
        return;
    }

    canvas_t backbuffer_canvas = {
        .buffer = (uint32_t *)backbuffer,
        .size = (rect_size_t){active_mode.width, active_mode.height},
        .pitch = active_mode.pitch};
    canvas_clear(&backbuffer_canvas, COLOR_BLACK);

    irq_flags_t rflags = spinlock_acquire_irqsave(&layer_list_lock);
    layer_t *current = layer_head;

    while (current != NULL) {
        layer_t *next = current->next;
        spinlock_release_irqrestore(&layer_list_lock, rflags);

        composite_layer(current);

        rflags = spinlock_acquire_irqsave(&layer_list_lock);
        current = next;
    }
    spinlock_release_irqrestore(&layer_list_lock, rflags);
}

/** @brief Flushes the backbuffer to the physical framebuffer via the active video driver. */
static void compositor_swap() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    bool enabled = rendering_enabled;
    spinlock_release_irqrestore(&state_lock, sflags);
    if (!enabled || !backbuffer)
        return;

    mutex_lock(&compositor_mutex);
    gfx_flush(backbuffer, fb_size);
    mutex_unlock(&compositor_mutex);
}

/**
 * @brief Kernel thread function handling continuous compositing and frame rate limiting.
 *
 * @param[in] arg Unused argument pointer.
 */
static void compositor_thread(void *arg) {
    (void)arg;

    uint64_t last_fps_update = hpet_uptime_ms();
    uint64_t frame_count = 0;
    uint64_t last_frame_start = hpet_uptime_ms();

    while (1) {
        uint64_t frame_start = hpet_uptime_ms();

        uint64_t frame_delta_ms = frame_start - last_frame_start;
        if (frame_delta_ms == 0) {
            frame_delta_ms = 1;
        }

        irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
        bool initialized = compositor_initialized;
        double current_target_time = target_frame_time_ms;
        delta_time = (double)frame_delta_ms / 1000.0;
        spinlock_release_irqrestore(&state_lock, sflags);

        if (!initialized) {
            thread_exit();
        }

        last_frame_start = frame_start;

        compositor_compose();
        compositor_swap();

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

        if ((double)frame_duration < current_target_time) {
            uint64_t remaining_ms = (uint64_t)(current_target_time - (double)frame_duration);
            if (remaining_ms > 0) {
                thread_sleep_ms(remaining_ms);
            }
        } else {
            thread_yield();
        }
    }
}

/**
 * @brief Initializes the compositor subsystem, allocates the backbuffer, and spawns the render thread.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t compositor_init() {
    video_mode_t *mode = NULL;
    K_TRY(gfx_get_current_mode(&mode));
    if (!mode || mode->width == 0 || mode->height == 0) {
        log_error("current video mode is invalid or unavailable");
        return E_NOINIT;
    }

    active_mode = *mode;

    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    target_frame_time_ms = 1000.0 / (double)target_fps;
    spinlock_release_irqrestore(&state_lock, sflags);

    fb_size = (size_t)(active_mode.height * active_mode.pitch);
    log_var(fb_size);

    mutex_init(&compositor_mutex, "fb_hardware_lock");

    virt_addr_t mem = 0;
    K_TRY(kzalloc(fb_size, &mem));
    backbuffer = (uint8_t *)mem;

    sflags = spinlock_acquire_irqsave(&state_lock);
    compositor_initialized = true;
    spinlock_release_irqrestore(&state_lock, sflags);

    thread_t *thread = NULL;
    kerr_t err = thread_create(NULL, compositor_thread, NULL, "compositor_thread", &thread);
    if (k_failed(err)) {
        log_error("failed to create compositor thread (%s)", kerr_str(err));
        kfree((virt_addr_t)backbuffer);
        backbuffer = NULL;

        sflags = spinlock_acquire_irqsave(&state_lock);
        compositor_initialized = false;
        spinlock_release_irqrestore(&state_lock, sflags);

        return err;
    }

    log_debug("compositor initialized successfully");
    return E_OK;
}

/**
 * @brief Deinitializes the compositor and releases all allocated buffers.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t compositor_deinit() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    compositor_initialized = false;
    rendering_enabled = false;
    spinlock_release_irqrestore(&state_lock, sflags);

    if (backbuffer) {
        kfree((virt_addr_t)backbuffer);
        backbuffer = NULL;
    }

    log_info("compositor deinitialized");
    return E_OK;
}

/**
 * @brief Enables continuous rendering in the compositor loop.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t compositor_enable_rendering() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    rendering_enabled = true;
    spinlock_release_irqrestore(&state_lock, sflags);
    return E_OK;
}

/**
 * @brief Disables continuous rendering in the compositor loop.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t compositor_disable_rendering() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    rendering_enabled = false;
    spinlock_release_irqrestore(&state_lock, sflags);
    return E_OK;
}

/**
 * @brief Sets the target frame rate in frames per second.
 *
 * @param[in] fps Target FPS (must be greater than 0).
 * @return E_OK on success, or an error code on failure.
 */
kerr_t compositor_set_target_fps(uint64_t fps) {
    if (fps == 0) {
        return E_INVAL;
    }

    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    target_fps = fps;
    target_frame_time_ms = 1000.0 / (double)fps;
    spinlock_release_irqrestore(&state_lock, sflags);
    return E_OK;
}

/**
 * @brief Retrieves the measured frames per second over the last 1-second interval.
 *
 * @return Current frame rate in FPS.
 */
uint64_t compositor_get_current_fps() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    uint64_t fps = current_fps;
    spinlock_release_irqrestore(&state_lock, sflags);
    return fps;
}

/**
 * @brief Retrieves the delta time between the previous two frames in seconds.
 *
 * @return Delta time in seconds.
 */
double compositor_get_delta_time() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    double dt = delta_time;
    spinlock_release_irqrestore(&state_lock, sflags);
    return dt;
}

/**
 * @brief Retrieves the screen width of the active compositor video mode.
 *
 * @return Width in pixels.
 */
uint64_t compositor_get_width() {
    return active_mode.width;
}

/**
 * @brief Retrieves the screen height of the active compositor video mode.
 *
 * @return Height in pixels.
 */
uint64_t compositor_get_height() {
    return active_mode.height;
}

/**
 * @brief Retrieves the dimensions of the active compositor video mode.
 *
 * @return rect_size_t containing width and height.
 */
rect_size_t compositor_get_size() {
    return (rect_size_t){.width = active_mode.width, .height = active_mode.height};
}
