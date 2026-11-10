/**
 * @file version_splash.c
 * @brief Minimal NanoOS boot splash screen
 * @author friedrichOsDev
 */

#include <drivers/gfx/compositor.h>
#include <drivers/gfx/gfx.h>
#include <drivers/gfx/version_splash.h>
#include <lib/util/str.h>
#include <pch.h>
#include <sched/sched.h>

/**
 * @brief Displays a minimal, centered NanoOS splash screen using the built-in PSF2 font.
 *
 * @param[in] version_str Null-terminated version string (e.g., "0.0.3").
 * @param[in] duration_ms Duration in milliseconds to hold the splash display.
 * @return E_OK on success, E_INVAL for a NULL version, E_NOINIT if the screen or embedded font is unavailable, or another compositor error.
 */
kerr_t version_splash_show(const char *version_str, uint64_t duration_ms) {
    if (!version_str) {
        return E_INVAL;
    }

    rect_size_t screen = compositor_get_size();
    if (screen.width == 0U || screen.height == 0U ||
        screen.width > (uint64_t)INT64_MAX || screen.height > (uint64_t)INT64_MAX) {
        return E_NOINIT;
    }

    layer_t *splash_layer = NULL;
    K_TRY(layer_create(screen, 999, &splash_layer));

    kerr_t err = layer_draw_begin(splash_layer);
    if (k_failed(err)) {
        layer_destroy(splash_layer);
        return err;
    }

    canvas_t *canvas = &splash_layer->canvas;
    const int64_t center_x = (int64_t)(screen.width / 2U);
    const int64_t center_y = (int64_t)(screen.height / 2U);

    const color_t background = MAKE_COLOR(10, 14, 22);
    const color_t foreground = MAKE_COLOR(232, 240, 248);
    const color_t muted = MAKE_COLOR(112, 130, 150);
    const color_t accent = MAKE_COLOR(70, 220, 200);
    const color_t accent_dim = MAKE_COLOR(30, 76, 82);

    canvas_clear(canvas, background);

    /*
     * Keep the composition compact and centered. The bundled PSF2 font is
     * 8x16, so the title at scale 3 occupies 144x48 pixels.
     */
    const int64_t title_width = 6 * 8 * 3;
    const int64_t title_x = center_x - (title_width / 2);
    const int64_t title_y = center_y - 48;

    /* Small accent mark and a restrained divider. */
    const point_t accent_pos = {
        .x = center_x - 22,
        .y = title_y - 22};

    canvas_draw_rectangle(canvas, accent_pos, (rect_size_t){.width = 44U, .height = 3U}, accent, true, 0U);

    err = canvas_draw_text(canvas, (point_t){.x = title_x, .y = title_y}, "NanoOS", foreground, 3U);

    if (!k_failed(err)) {
        const size_t version_length = strlen(version_str);
        if (version_length <= (size_t)(INT64_MAX / 8)) {
            const int64_t version_width = (int64_t)(version_length * 8U);
            err = canvas_draw_text(
                canvas,
                (point_t){.x = center_x - (version_width / 2),
                          .y = title_y + 62},
                version_str, accent, 1U);
        } else {
            err = E_INVAL;
        }
    }

    if (!k_failed(err)) {
        const char *tagline = "A SMALL OPERATING SYSTEM";
        const int64_t tagline_width = 24 * 8;
        err = canvas_draw_text(canvas, (point_t){.x = center_x - (tagline_width / 2), .y = title_y + 88}, tagline, muted, 1U);
    }

    canvas_draw_rectangle(canvas, (point_t){.x = center_x - 48, .y = title_y + 116}, (rect_size_t){.width = 96U, .height = 1U}, accent_dim, true, 0U);

    layer_draw_end(splash_layer);

    if (!k_failed(err)) {
        thread_sleep_ms(duration_ms);
    }

    layer_destroy(splash_layer);
    return err;
}
