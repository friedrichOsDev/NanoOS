/**
 * @file compositor.c
 * @brief 2D software compositor/renderer
 * @author friedrichOsDev
 */

#include "font.h"
#include <arch/arch.h>
#include <drivers/gfx/compositor.h>
#include <drivers/gfx/gfx.h>
#include <kernel/sync.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <pch.h>
#include <sched/sched.h>
#include <sched/thread.h>

#define MAX_COMPOSITOR_LAYERS 128

static psf2_font_t builtin_font;
static bool builtin_font_checked = false;
static bool builtin_font_valid = false;

static bool compositor_initialized = false;
static bool rendering_enabled = false;
static uint8_t *backbuffer = NULL;
static size_t fb_size = 0;
static video_mode_t active_mode = {0};
static layer_t *layer_head = NULL;

static mutex_t compositor_mutex;
static spinlock_t layer_list_lock = SPINLOCK_INIT;
static spinlock_t state_lock = SPINLOCK_INIT;
static spinlock_t compositor_dirty_lock = SPINLOCK_INIT;

static volatile bool compositor_dirty = false;
static rect_t global_dirty_rect = {0};
static rect_t last_composed_rect = {0};

static uint64_t target_fps = 60;
static double target_frame_time_ms = 0.0;
static uint64_t current_fps = 0;
static double delta_time = 0.0;

/**
 * @brief Computes the integer square root of a 64-bit integer using bitwise operations.
 *
 * @param[in] val Value to take the square root of.
 * @return Square root rounded down to the nearest integer.
 */
static inline uint64_t int_sqrt(uint64_t val) {
    uint64_t res = 0;
    uint64_t bit = 1ULL << 62;
    while (bit > val) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (val >= res + bit) {
            val -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

/**
 * @brief Computes the minimal bounding box containing both rectangles.
 *
 * @param[in] r1 First rectangle.
 * @param[in] r2 Second rectangle.
 * @return Union bounding box.
 */
static inline rect_t rect_union(rect_t r1, rect_t r2) {
    if (r1.size.width == 0 || r1.size.height == 0) {
        return r2;
    }
    if (r2.size.width == 0 || r2.size.height == 0) {
        return r1;
    }

    int64_t x1 = (r1.pos.x < r2.pos.x) ? r1.pos.x : r2.pos.x;
    int64_t y1 = (r1.pos.y < r2.pos.y) ? r1.pos.y : r2.pos.y;

    int64_t r1_right = r1.pos.x + (int64_t)r1.size.width;
    int64_t r2_right = r2.pos.x + (int64_t)r2.size.width;
    int64_t x2 = (r1_right > r2_right) ? r1_right : r2_right;

    int64_t r1_bottom = r1.pos.y + (int64_t)r1.size.height;
    int64_t r2_bottom = r2.pos.y + (int64_t)r2.size.height;
    int64_t y2 = (r1_bottom > r2_bottom) ? r1_bottom : r2_bottom;

    return (rect_t){
        .pos = {x1, y1},
        .size = {(uint64_t)(x2 - x1), (uint64_t)(y2 - y1)}};
}

/**
 * @brief Computes the intersection rectangle of two rectangles.
 *
 * @param[in] r1 First rectangle.
 * @param[in] r2 Second rectangle.
 * @param[out] out Intersection rectangle.
 * @return True if the rectangles overlap, false otherwise.
 */
static inline bool rect_intersect(rect_t r1, rect_t r2, rect_t *out) {
    int64_t x1 = (r1.pos.x > r2.pos.x) ? r1.pos.x : r2.pos.x;
    int64_t y1 = (r1.pos.y > r2.pos.y) ? r1.pos.y : r2.pos.y;

    int64_t r1_right = r1.pos.x + (int64_t)r1.size.width;
    int64_t r2_right = r2.pos.x + (int64_t)r2.size.width;
    int64_t x2 = (r1_right < r2_right) ? r1_right : r2_right;

    int64_t r1_bottom = r1.pos.y + (int64_t)r1.size.height;
    int64_t r2_bottom = r2.pos.y + (int64_t)r2.size.height;
    int64_t y2 = (r1_bottom < r2_bottom) ? r1_bottom : r2_bottom;

    if (x2 <= x1 || y2 <= y1) {
        if (out) {
            *out = (rect_t){0};
        }
        return false;
    }

    if (out) {
        out->pos = (point_t){x1, y1};
        out->size = (rect_size_t){(uint64_t)(x2 - x1), (uint64_t)(y2 - y1)};
    }
    return true;
}

/**
 * @brief Marks a rectangular screen region dirty; the compositor thread polls this flag
 *        and composites on the next frame boundary.
 *
 * @param[in] rect Screen-space rectangle requiring compositing.
 */
static void compositor_mark_dirty_rect(rect_t rect) {
    if (rect.size.width == 0 || rect.size.height == 0) {
        return;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&compositor_dirty_lock);
    if (!compositor_dirty || global_dirty_rect.size.width == 0 || global_dirty_rect.size.height == 0) {
        global_dirty_rect = rect;
    } else {
        global_dirty_rect = rect_union(global_dirty_rect, rect);
    }
    compositor_dirty = true;
    spinlock_release_irqrestore(&compositor_dirty_lock, flags);
}

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
    if (!canvas || !canvas->buffer) {
        return;
    }

    int64_t clip_x1 = 0;
    int64_t clip_x2 = (int64_t)canvas->size.width - 1;
    int64_t clip_y1 = 0;
    int64_t clip_y2 = (int64_t)canvas->size.height - 1;

    if (canvas->clip_enabled) {
        if (canvas->clip_rect.pos.x > clip_x1) {
            clip_x1 = canvas->clip_rect.pos.x;
        }
        int64_t r_x2 = canvas->clip_rect.pos.x + (int64_t)canvas->clip_rect.size.width - 1;
        if (r_x2 < clip_x2) {
            clip_x2 = r_x2;
        }

        if (canvas->clip_rect.pos.y > clip_y1) {
            clip_y1 = canvas->clip_rect.pos.y;
        }
        int64_t r_y2 = canvas->clip_rect.pos.y + (int64_t)canvas->clip_rect.size.height - 1;
        if (r_y2 < clip_y2) {
            clip_y2 = r_y2;
        }
    }

    if (clip_x1 > clip_x2 || clip_y1 > clip_y2 || y < clip_y1 || y > clip_y2) {
        return;
    }

    if (x1 > x2) {
        int64_t t = x1;
        x1 = x2;
        x2 = t;
    }
    if (x2 < clip_x1 || x1 > clip_x2) {
        return;
    }

    if (x1 < clip_x1) {
        x1 = clip_x1;
    }
    if (x2 > clip_x2) {
        x2 = clip_x2;
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
    canvas->clip_rect = (rect_t){.pos = {0, 0}, .size = size};
    canvas->clip_enabled = false;

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
 * @brief Clears the entire canvas (or active clipping region) using the specified color.
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] color Fill color.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_clear(canvas_t *canvas, color_t color) {
    if (!canvas || !canvas->buffer) {
        return E_INVAL;
    }

    if (canvas->clip_enabled) {
        rect_t canvas_bound = (rect_t){.pos = {0, 0}, .size = canvas->size};
        rect_t clipped;
        if (!rect_intersect(canvas->clip_rect, canvas_bound, &clipped)) {
            return E_OK;
        }

        uint32_t color_val = pack_color(color);
        size_t stride = (size_t)(canvas->pitch / sizeof(uint32_t));
        for (uint64_t row = 0; row < clipped.size.height; row++) {
            int64_t y = clipped.pos.y + (int64_t)row;
            uint32_t *line = canvas->buffer + ((size_t)y * stride) + (size_t)clipped.pos.x;
            memset32(line, color_val, (size_t)clipped.size.width);
        }
        return E_OK;
    }

    uint32_t color_val = pack_color(color);
    size_t count = (size_t)(canvas->size.width * canvas->size.height);
    memset32(canvas->buffer, color_val, count);
    return E_OK;
}

/**
 * @brief Draws a single pixel onto the canvas with clipping and alpha blending.
 *
 * @param[in,out] canvas Pointer to target canvas.
 * @param[in] pos Coordinate position.
 * @param[in] color Pixel color.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_draw_pixel(canvas_t *canvas, point_t pos, color_t color) {
    if (!canvas || !canvas->buffer) {
        return E_INVAL;
    }

    int64_t clip_x1 = 0;
    int64_t clip_x2 = (int64_t)canvas->size.width - 1;
    int64_t clip_y1 = 0;
    int64_t clip_y2 = (int64_t)canvas->size.height - 1;

    if (canvas->clip_enabled) {
        if (canvas->clip_rect.pos.x > clip_x1) {
            clip_x1 = canvas->clip_rect.pos.x;
        }
        int64_t r_x2 = canvas->clip_rect.pos.x + (int64_t)canvas->clip_rect.size.width - 1;
        if (r_x2 < clip_x2) {
            clip_x2 = r_x2;
        }

        if (canvas->clip_rect.pos.y > clip_y1) {
            clip_y1 = canvas->clip_rect.pos.y;
        }
        int64_t r_y2 = canvas->clip_rect.pos.y + (int64_t)canvas->clip_rect.size.height - 1;
        if (r_y2 < clip_y2) {
            clip_y2 = r_y2;
        }
    }

    if (pos.x < clip_x1 || pos.x > clip_x2 || pos.y < clip_y1 || pos.y > clip_y2) {
        return E_OK;
    }

    size_t pixels_per_row = (size_t)(canvas->pitch / sizeof(uint32_t));
    size_t offset = ((size_t)pos.y * pixels_per_row) + (size_t)pos.x;
    color_t bg_color = unpack_color(canvas->buffer[offset]);
    canvas->buffer[offset] = pack_color(blend_color(bg_color, color));
    return E_OK;
}

/**
 * @brief Configures an active clipping rectangle for the canvas.
 *
 * @param[in,out] canvas Target canvas.
 * @param[in] clip_rect Clipping rectangle in canvas coordinates.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_set_clip_rect(canvas_t *canvas, rect_t clip_rect) {
    if (!canvas) {
        return E_INVAL;
    }

    rect_t canvas_rect = (rect_t){.pos = {0, 0}, .size = canvas->size};
    rect_t clipped;
    if (!rect_intersect(clip_rect, canvas_rect, &clipped)) {
        canvas->clip_rect = (rect_t){0};
    } else {
        canvas->clip_rect = clipped;
    }
    canvas->clip_enabled = true;
    return E_OK;
}

/**
 * @brief Resets and disables the canvas clipping rectangle.
 *
 * @param[in,out] canvas Target canvas.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_reset_clip_rect(canvas_t *canvas) {
    if (!canvas) {
        return E_INVAL;
    }

    canvas->clip_rect = (rect_t){.pos = {0, 0}, .size = canvas->size};
    canvas->clip_enabled = false;
    return E_OK;
}

/**
 * @brief Retrieves the active canvas clipping rectangle.
 *
 * @param[in] canvas Target canvas.
 * @param[out] out Pointer to store the active clipping rectangle.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t canvas_get_clip_rect(const canvas_t *canvas, rect_t *out) {
    if (!canvas || !out) {
        return E_INVAL;
    }

    *out = canvas->clip_rect;
    return E_OK;
}

/**
 * @brief Read a 32-bit value from a font location.
 *
 * @param[in] ptr Pointer to the font data.
 * @return The 32-bit value read from the font location.
 */
static uint32_t font_read_u32(const uint8_t *ptr) {
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

/**
 * @brief Initialize the font system.
 *
 * @return true if the font system is initialized successfully, false otherwise.
 */
static bool font_init() {
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
        .end = end};

    if ((flags & 1U) != 0U && builtin_font.unicode_table >= end) {
        return false;
    }

    builtin_font_valid = true;
    return true;
}

/**
 * @brief Decode a UTF-8 character.
 *
 * @param[in] text The text to decode.
 * @param[in] remaining The remaining bytes to decode.
 * @param[out] consumed The number of bytes consumed.
 * @return The decoded character.
 */
static uint32_t font_utf8_decode(const uint8_t *text, size_t remaining, size_t *consumed) {
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

/**
 * @brief Find a glyph in the font.
 *
 * @param codepoint The Unicode codepoint to find.
 * @return The index of the glyph in the font, or 0 if not found.
 */
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
            uint32_t current = font_utf8_decode(cursor, (size_t)(end - cursor), &consumed);
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
 * @brief Draws a UTF-8 string using the embedded PSF2 bitmap font with full clipping and opacity fast-path.
 *
 * @param[in,out] canvas Target canvas.
 * @param[in] pos Top-left position.
 * @param[in] text Null-terminated UTF-8 string.
 * @param[in] color Foreground color.
 * @param[in] scale Integer scale from 1 to 16.
 * @return E_OK on success, or an error code.
 */
kerr_t canvas_draw_text(canvas_t *canvas, point_t pos, const char *text, color_t color, uint32_t scale) {
    if (!canvas || !canvas->buffer || !text || scale == 0U || scale > 16U) {
        return E_INVAL;
    }
    if (!font_init()) {
        return E_NOINIT;
    }
    if (color.a == 0U) {
        return E_OK;
    }

    int64_t clip_x1 = 0;
    int64_t clip_x2 = (int64_t)canvas->size.width - 1;
    int64_t clip_y1 = 0;
    int64_t clip_y2 = (int64_t)canvas->size.height - 1;

    if (canvas->clip_enabled) {
        if (canvas->clip_rect.pos.x > clip_x1) {
            clip_x1 = canvas->clip_rect.pos.x;
        }
        int64_t r_x2 = canvas->clip_rect.pos.x + (int64_t)canvas->clip_rect.size.width - 1;
        if (r_x2 < clip_x2) {
            clip_x2 = r_x2;
        }

        if (canvas->clip_rect.pos.y > clip_y1) {
            clip_y1 = canvas->clip_rect.pos.y;
        }
        int64_t r_y2 = canvas->clip_rect.pos.y + (int64_t)canvas->clip_rect.size.height - 1;
        if (r_y2 < clip_y2) {
            clip_y2 = r_y2;
        }
    }

    if (clip_x1 > clip_x2 || clip_y1 > clip_y2) {
        return E_OK;
    }

    uint32_t packed_color = pack_color(color);
    int64_t origin_x = pos.x;
    int64_t cursor_x = pos.x;
    int64_t cursor_y = pos.y;
    int64_t advance_x = (int64_t)(builtin_font.width * scale);
    int64_t advance_y = (int64_t)(builtin_font.height * scale);
    int64_t tab_width = advance_x * 4;

    const uint8_t *input = (const uint8_t *)text;
    while (*input != 0U) {
        size_t available = 1U;
        while (available < 4U && input[available] != 0U) {
            available++;
        }
        size_t consumed = 0U;
        uint32_t codepoint = font_utf8_decode(input, available, &consumed);
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
            int64_t tab_remainder = relative % tab_width;
            int64_t tab_advance = tab_width - tab_remainder;
            if (cursor_x > INT64_MAX - tab_advance) {
                break;
            }
            cursor_x += tab_advance;
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
                    if (y < clip_y1 || y > clip_y2) {
                        continue;
                    }
                    size_t stride = (size_t)(canvas->pitch / sizeof(uint32_t));
                    uint32_t *row = canvas->buffer + ((size_t)y * stride);
                    for (uint32_t sx = 0U; sx < scale; sx++) {
                        int64_t px = pixel_x + (int64_t)sx;
                        if (px < clip_x1 || px > clip_x2) {
                            continue;
                        }
                        size_t offset = (size_t)px;
                        if (color.a == 255) {
                            row[offset] = packed_color;
                        } else {
                            color_t background = unpack_color(row[offset]);
                            row[offset] = pack_color(blend_color(background, color));
                        }
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
    cur_x2 = v2.x << 16;
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
 * @brief Draws a line between two points using Bresenham algorithm or scanline polygon rasterization for thick lines.
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

    if (thickness == 1) {
        int64_t dx = (end.x > start.x) ? (end.x - start.x) : (start.x - end.x);
        int64_t dy = (end.y > start.y) ? (end.y - start.y) : (start.y - end.y);
        int64_t sx = (start.x < end.x) ? 1 : -1;
        int64_t sy = (start.y < end.y) ? 1 : -1;
        int64_t err = dx - dy;

        while (1) {
            canvas_draw_pixel(canvas, start, color);
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

    /* Single point thick line */
    if (start.x == end.x && start.y == end.y) {
        return canvas_draw_circle(canvas, start, thickness / 2, color, true, 0);
    }

    /* Scanline polygon rasterization for thick lines without overdraw */
    int64_t dx = end.x - start.x;
    int64_t dy = end.y - start.y;
    uint64_t len_sq = (uint64_t)(dx * dx + dy * dy);
    uint64_t len = int_sqrt(len_sq);
    if (len == 0) {
        len = 1;
    }

    int64_t half_th = (int64_t)thickness;
    int64_t nx = (-dy * half_th) / (2 * (int64_t)len);
    int64_t ny = (dx * half_th) / (2 * (int64_t)len);

    if (nx == 0 && ny == 0) {
        if ((dx >= 0 ? dx : -dx) >= (dy >= 0 ? dy : -dy)) {
            ny = (dy >= 0) ? 1 : -1;
        } else {
            nx = (dx >= 0) ? -1 : 1;
        }
    }

    point_t poly[4] = {
        {start.x + nx, start.y + ny},
        {start.x - nx, start.y - ny},
        {end.x - nx, end.y - ny},
        {end.x + nx, end.y + ny}};

    int64_t min_y = poly[0].y;
    int64_t max_y = poly[0].y;
    for (int i = 1; i < 4; i++) {
        if (poly[i].y < min_y) {
            min_y = poly[i].y;
        }
        if (poly[i].y > max_y) {
            max_y = poly[i].y;
        }
    }

    for (int64_t y = min_y; y <= max_y; y++) {
        int64_t line_min_x = INT64_MAX;
        int64_t line_max_x = INT64_MIN;

        for (int i = 0; i < 4; i++) {
            point_t p1 = poly[i];
            point_t p2 = poly[(i + 1) % 4];

            if (p1.y == p2.y) {
                if (p1.y == y) {
                    int64_t h_min = (p1.x < p2.x) ? p1.x : p2.x;
                    int64_t h_max = (p1.x > p2.x) ? p1.x : p2.x;
                    if (h_min < line_min_x) {
                        line_min_x = h_min;
                    }
                    if (h_max > line_max_x) {
                        line_max_x = h_max;
                    }
                }
                continue;
            }

            int64_t y_low = (p1.y < p2.y) ? p1.y : p2.y;
            int64_t y_high = (p1.y > p2.y) ? p1.y : p2.y;

            if (y >= y_low && y <= y_high) {
                int64_t inter_x = p1.x + ((y - p1.y) * (p2.x - p1.x)) / (p2.y - p1.y);
                if (inter_x < line_min_x) {
                    line_min_x = inter_x;
                }
                if (inter_x > line_max_x) {
                    line_max_x = inter_x;
                }
            }
        }

        if (line_min_x <= line_max_x) {
            canvas_draw_hline(canvas, line_min_x, line_max_x, y, color);
        }
    }

    return E_OK;
}

/**
 * @brief Draws a circle centered at a specific point using pure integer midpoint scanlines.
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

    int64_t x_outer = r_outer;
    int64_t x_inner = r_inner;

    for (int64_t y = 0; y <= r_outer; y++) {
        int64_t y_sq = y * y;

        while (x_outer > 0 && (x_outer * x_outer + y_sq > r_outer_sq)) {
            x_outer--;
        }

        if (r_inner > 0 && y < r_inner) {
            while (x_inner > 0 && (x_inner * x_inner + y_sq >= r_inner_sq)) {
                x_inner--;
            }
        } else {
            x_inner = 0;
        }

        if (x_inner > 0) {
            canvas_draw_hline(canvas, center.x - x_outer, center.x - x_inner, center.y + y, color);
            canvas_draw_hline(canvas, center.x + x_inner, center.x + x_outer, center.y + y, color);
            if (y > 0) {
                canvas_draw_hline(canvas, center.x - x_outer, center.x - x_inner, center.y - y, color);
                canvas_draw_hline(canvas, center.x + x_inner, center.x + x_outer, center.y - y, color);
            }
        } else {
            canvas_draw_hline(canvas, center.x - x_outer, center.x + x_outer, center.y + y, color);
            if (y > 0) {
                canvas_draw_hline(canvas, center.x - x_outer, center.x + x_outer, center.y - y, color);
            }
        }
    }

    return E_OK;
}

/**
 * @brief Increments reference count of a layer.
 *
 * @param[in,out] layer Target layer.
 */
static void layer_ref(layer_t *layer) {
    if (!layer) {
        return;
    }
    __atomic_add_fetch(&layer->ref_count, 1, __ATOMIC_RELAXED);
}

/**
 * @brief Releases internal canvas buffer and layer allocation.
 *
 * @param[in,out] layer Target layer to free.
 */
static void layer_free(layer_t *layer) {
    if (!layer) {
        return;
    }
    if (layer->canvas.buffer) {
        kfree((virt_addr_t)layer->canvas.buffer);
        layer->canvas.buffer = NULL;
    }
    kfree((virt_addr_t)layer);
}

/**
 * @brief Decrements reference count of a layer and frees it when reaching zero.
 *
 * @param[in,out] layer Target layer.
 */
static void layer_unref(layer_t *layer) {
    if (!layer) {
        return;
    }
    if (__atomic_sub_fetch(&layer->ref_count, 1, __ATOMIC_ACQ_REL) == 0) {
        layer_free(layer);
    }
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
    layer->opaque = false;
    layer->next = NULL;
    layer->ref_count = 1;
    layer->dirty_rect = (rect_t){0};
    layer->has_partial_dirty = false;
    mutex_init(&layer->lock, "layer_mutex");

    insert_layer_ordered(layer);
    compositor_mark_dirty_rect((rect_t){layer->pos, layer->canvas.size});

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

    rect_t dirty = (rect_t){layer->pos, layer->canvas.size};

    remove_layer_ordered(layer);

    mutex_lock(&layer->lock);
    layer->visible = false;
    mutex_unlock(&layer->lock);

    compositor_mark_dirty_rect(dirty);
    layer_unref(layer);
    return E_OK;
}

/**
 * @brief Sets the on-screen position of a layer and marks the old and new areas dirty.
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
    point_t old_pos = layer->pos;
    rect_size_t size = layer->canvas.size;
    layer->pos = pos;
    bool visible = layer->visible;
    mutex_unlock(&layer->lock);

    if (visible) {
        rect_t old_r = (rect_t){old_pos, size};
        rect_t new_r = (rect_t){pos, size};
        compositor_mark_dirty_rect(rect_union(old_r, new_r));
    }

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
    rect_size_t old_size = layer->canvas.size;
    point_t pos = layer->pos;
    bool visible = layer->visible;

    if (layer->canvas.buffer) {
        kfree((virt_addr_t)layer->canvas.buffer);
    }
    layer->canvas.buffer = (uint32_t *)mem;
    layer->canvas.size = size;
    layer->canvas.pitch = size.width * sizeof(uint32_t);
    layer->canvas.clip_rect = (rect_t){.pos = {0, 0}, .size = size};
    layer->canvas.clip_enabled = false;
    mutex_unlock(&layer->lock);

    if (visible) {
        rect_t old_r = (rect_t){pos, old_size};
        rect_t new_r = (rect_t){pos, size};
        compositor_mark_dirty_rect(rect_union(old_r, new_r));
    }

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
    bool changed = (layer->visible != visible);
    layer->visible = visible;
    rect_t dirty = (rect_t){layer->pos, layer->canvas.size};
    mutex_unlock(&layer->lock);

    if (changed) {
        compositor_mark_dirty_rect(dirty);
    }

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
    rect_t dirty = (rect_t){layer->pos, layer->canvas.size};
    bool visible = layer->visible;
    mutex_unlock(&layer->lock);

    insert_layer_ordered(layer);

    if (visible) {
        compositor_mark_dirty_rect(dirty);
    }

    return E_OK;
}

/**
 * @brief Configures whether a layer is completely opaque for fast-path copying.
 *
 * @param[in,out] layer Pointer to layer.
 * @param[in] opaque True if the layer contains only opaque pixels.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_set_opaque(layer_t *layer, bool opaque) {
    if (!layer) {
        return E_INVAL;
    }

    mutex_lock(&layer->lock);
    layer->opaque = opaque;
    rect_t dirty = (rect_t){layer->pos, layer->canvas.size};
    bool visible = layer->visible;
    mutex_unlock(&layer->lock);

    if (visible) {
        compositor_mark_dirty_rect(dirty);
    }

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
    layer->has_partial_dirty = false;
    layer->dirty_rect = (rect_t){0};
    return E_OK;
}

/**
 * @brief Releases the layer lock and marks dirty areas for presentation.
 *
 * @param[in,out] layer Pointer to layer.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_draw_end(layer_t *layer) {
    if (!layer) {
        return E_INVAL;
    }

    rect_t dirty;
    if (layer->has_partial_dirty && layer->dirty_rect.size.width > 0 && layer->dirty_rect.size.height > 0) {
        dirty = (rect_t){
            .pos = {layer->pos.x + layer->dirty_rect.pos.x, layer->pos.y + layer->dirty_rect.pos.y},
            .size = layer->dirty_rect.size};
    } else {
        dirty = (rect_t){layer->pos, layer->canvas.size};
    }

    bool visible = layer->visible;
    layer->has_partial_dirty = false;
    layer->dirty_rect = (rect_t){0};
    mutex_unlock(&layer->lock);

    if (visible) {
        compositor_mark_dirty_rect(dirty);
    }

    return E_OK;
}

/**
 * @brief Invalidates the entire layer, requesting a redraw by the compositor.
 *
 * @param[in,out] layer Target layer.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_invalidate(layer_t *layer) {
    if (!layer) {
        return E_INVAL;
    }

    mutex_lock(&layer->lock);
    rect_t dirty = (rect_t){layer->pos, layer->canvas.size};
    bool visible = layer->visible;
    layer->has_partial_dirty = false;
    layer->dirty_rect = (rect_t){0};
    mutex_unlock(&layer->lock);

    if (visible) {
        compositor_mark_dirty_rect(dirty);
    }

    return E_OK;
}

/**
 * @brief Invalidates a sub-rectangle within a layer, triggering partial compositing.
 *
 * @param[in,out] layer Target layer.
 * @param[in] rect Sub-rectangle in layer-local coordinates.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t layer_invalidate_rect(layer_t *layer, rect_t rect) {
    if (!layer) {
        return E_INVAL;
    }

    mutex_lock(&layer->lock);
    rect_t canvas_rect = (rect_t){.pos = {0, 0}, .size = layer->canvas.size};
    rect_t clipped;
    if (!rect_intersect(rect, canvas_rect, &clipped)) {
        mutex_unlock(&layer->lock);
        return E_OK;
    }

    if (!layer->has_partial_dirty || layer->dirty_rect.size.width == 0) {
        layer->dirty_rect = clipped;
    } else {
        layer->dirty_rect = rect_union(layer->dirty_rect, clipped);
    }
    layer->has_partial_dirty = true;

    rect_t screen_dirty = (rect_t){
        .pos = {layer->pos.x + clipped.pos.x, layer->pos.y + clipped.pos.y},
        .size = clipped.size};
    bool visible = layer->visible;
    mutex_unlock(&layer->lock);

    if (visible) {
        compositor_mark_dirty_rect(screen_dirty);
    }

    return E_OK;
}

/**
 * @brief Composites a single layer into the backbuffer clipped against a specific dirty region.
 *
 * @param[in,out] layer Layer to composite.
 * @param[in] clip_dirty Dirty rectangle in screen coordinates.
 */
static void composite_layer_clipped(layer_t *layer, rect_t clip_dirty) {
    if (!layer || !layer->visible || !layer->canvas.buffer) {
        return;
    }

    canvas_t *layer_canvas = &layer->canvas;
    rect_t layer_screen_rect = (rect_t){.pos = layer->pos, .size = layer_canvas->size};

    rect_t target_rect;
    if (!rect_intersect(layer_screen_rect, clip_dirty, &target_rect)) {
        return;
    }

    size_t dest_pitch_dwords = active_mode.pitch / sizeof(uint32_t);
    size_t src_pitch_dwords = layer_canvas->pitch / sizeof(uint32_t);
    uint32_t *bb32 = (uint32_t *)(void *)backbuffer;

    int64_t lx_start = target_rect.pos.x - layer->pos.x;
    int64_t ly_start = target_rect.pos.y - layer->pos.y;

    if (layer->opaque) {
        for (uint64_t row = 0; row < target_rect.size.height; row++) {
            int64_t screen_y = target_rect.pos.y + (int64_t)row;
            int64_t ly = ly_start + (int64_t)row;

            uint32_t *dest_row = bb32 + ((size_t)screen_y * dest_pitch_dwords);
            const uint32_t *src_row = layer_canvas->buffer + ((size_t)ly * src_pitch_dwords);

            memcpy(&dest_row[target_rect.pos.x], &src_row[lx_start], (size_t)target_rect.size.width * sizeof(uint32_t));
        }
        return;
    }

    for (uint64_t row = 0; row < target_rect.size.height; row++) {
        int64_t screen_y = target_rect.pos.y + (int64_t)row;
        int64_t ly = ly_start + (int64_t)row;

        uint32_t *dest_row = bb32 + ((size_t)screen_y * dest_pitch_dwords);
        const uint32_t *src_row = layer_canvas->buffer + ((size_t)ly * src_pitch_dwords);

        for (uint64_t col = 0; col < target_rect.size.width; col++) {
            int64_t screen_x = target_rect.pos.x + (int64_t)col;
            int64_t lx = lx_start + (int64_t)col;

            uint32_t src_val = src_row[lx];
            uint32_t alpha = (src_val >> 24) & 0xFF;

            if (alpha == 255) {
                dest_row[screen_x] = src_val;
            } else if (alpha > 0) {
                color_t bg = unpack_color(dest_row[screen_x]);
                color_t fg = unpack_color(src_val);
                dest_row[screen_x] = pack_color(blend_color(bg, fg));
            }
        }
    }
}

/**
 * @brief Composites all visible layers intersecting the active dirty bounding box into the backbuffer.
 */
static void compositor_compose() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    bool enabled = rendering_enabled;
    spinlock_release_irqrestore(&state_lock, sflags);

    if (!enabled || !backbuffer) {
        return;
    }

    irq_flags_t dflags = spinlock_acquire_irqsave(&compositor_dirty_lock);
    if (!compositor_dirty) {
        spinlock_release_irqrestore(&compositor_dirty_lock, dflags);
        return;
    }
    rect_t dirty = global_dirty_rect;
    global_dirty_rect = (rect_t){0};
    compositor_dirty = false;
    spinlock_release_irqrestore(&compositor_dirty_lock, dflags);

    rect_t screen_rect = (rect_t){.pos = {0, 0}, .size = {active_mode.width, active_mode.height}};
    rect_t clip_dirty;
    if (!rect_intersect(dirty, screen_rect, &clip_dirty) || clip_dirty.size.width == 0 || clip_dirty.size.height == 0) {
        return;
    }

    /* Clear only the dirty sub-rectangle in the backbuffer to background color */
    size_t stride_dwords = active_mode.pitch / sizeof(uint32_t);
    uint32_t *bb32 = (uint32_t *)(void *)backbuffer;
    uint32_t black = pack_color(COLOR_BLACK);
    for (uint64_t y = 0; y < clip_dirty.size.height; y++) {
        int64_t screen_y = clip_dirty.pos.y + (int64_t)y;
        uint32_t *row = bb32 + ((size_t)screen_y * stride_dwords) + (size_t)clip_dirty.pos.x;
        memset32(row, black, (size_t)clip_dirty.size.width);
    }

    /* Snapshot layers with reference counting to prevent use-after-free */
    layer_t *render_list[MAX_COMPOSITOR_LAYERS];
    size_t render_count = 0;

    irq_flags_t rflags = spinlock_acquire_irqsave(&layer_list_lock);
    for (layer_t *curr = layer_head; curr != NULL && render_count < MAX_COMPOSITOR_LAYERS; curr = curr->next) {
        if (curr->visible) {
            layer_ref(curr);
            render_list[render_count++] = curr;
        }
    }
    spinlock_release_irqrestore(&layer_list_lock, rflags);

    /* Composite layers in ascending Z-order */
    for (size_t i = 0; i < render_count; i++) {
        layer_t *layer = render_list[i];

        mutex_lock(&layer->lock);
        if (layer->visible && layer->canvas.buffer) {
            composite_layer_clipped(layer, clip_dirty);
        }
        mutex_unlock(&layer->lock);

        layer_unref(layer);
    }

    last_composed_rect = clip_dirty;
}

/**
 * @brief Flushes the composed backbuffer region to the physical framebuffer via the active video driver.
 */
static void compositor_swap() {
    irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
    bool enabled = rendering_enabled;
    spinlock_release_irqrestore(&state_lock, sflags);
    if (!enabled || !backbuffer) {
        return;
    }

    mutex_lock(&compositor_mutex);
    if (last_composed_rect.size.width > 0 && last_composed_rect.size.height > 0) {
        gfx_flush_rect(backbuffer, active_mode.pitch,
                       (uint64_t)last_composed_rect.pos.x,
                       (uint64_t)last_composed_rect.pos.y,
                       last_composed_rect.size.width,
                       last_composed_rect.size.height);
    } else {
        gfx_flush(backbuffer, fb_size);
    }
    mutex_unlock(&compositor_mutex);
}

/**
 * @brief Kernel thread function handling polling-based compositing and frame rate pacing.
 *
 * The thread runs at a fixed frame rate (default: 60 FPS). It checks the dirty flag each
 * iteration and composes + swaps only when content has changed. This avoids the scheduling
 * races that arise from manually setting thread state to THREAD_BLOCKED outside the
 * scheduler lock.
 *
 * @param[in] arg Unused argument pointer.
 */
static void compositor_thread(void *arg) {
    (void)arg;

    uint64_t last_fps_update = hpet_uptime_ms();
    uint64_t frame_count = 0;
    uint64_t last_frame_start = hpet_uptime_ms();

    while (1) {
        irq_flags_t sflags = spinlock_acquire_irqsave(&state_lock);
        bool initialized = compositor_initialized;
        double current_target_time = target_frame_time_ms;
        spinlock_release_irqrestore(&state_lock, sflags);

        if (!initialized) {
            thread_exit();
        }

        uint64_t frame_start = hpet_uptime_ms();
        uint64_t frame_delta_ms = frame_start - last_frame_start;
        if (frame_delta_ms == 0) {
            frame_delta_ms = 1;
        }
        last_frame_start = frame_start;

        sflags = spinlock_acquire_irqsave(&state_lock);
        delta_time = (double)frame_delta_ms / 1000.0;
        spinlock_release_irqrestore(&state_lock, sflags);

        log_debug("Frame start: %llu, Delta time: %f", frame_start, frame_delta_ms);
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

    if (!font_init()) {
        log_warn("embedded PSF2 font is unavailable or invalid");
    }

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

    compositor_present();

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

    compositor_mark_dirty_rect((rect_t){.pos = {0, 0}, .size = {active_mode.width, active_mode.height}});

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
    compositor_present();
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

/**
 * @brief Requests immediate full-screen compositing and presentation.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t compositor_present() {
    compositor_mark_dirty_rect((rect_t){.pos = {0, 0}, .size = {active_mode.width, active_mode.height}});
    return E_OK;
}
