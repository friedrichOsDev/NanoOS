/**
 * @file shell_renderer.c
 * @brief Kernel shell rendering subsystem.
 * @author friedrichOsDev
 */

#include "shell_renderer.h"
#include "shell_internal.h"
#include <drivers/gfx/compositor.h>
#include <sched/sched.h>
#include <shell/shell.h>

static void shell_render_frame() {
    shell_screen_t *shell_screen = shell_get_screen();

    layer_t *layer = shell_get_layer();
    if (!layer) return;

    rect_size_t size = layer->canvas.size;
    if (size.width == 0U || size.height < 48U) {
        return;
    }

    if (k_failed(layer_draw_begin(layer))) return;

    canvas_t *canvas = &layer->canvas;
    const color_t shell_background = MAKE_COLOR(0, 0, 0);
    const color_t shell_foreground = MAKE_COLOR(224, 228, 232);
    // const color_t shell_cursor_color = MAKE_COLOR(224, 228, 232);

    // BG
    canvas_clear(canvas, shell_background);

    // FG
    if (shell_screen->dirty == false) {
        if (k_failed(layer_draw_end(layer))) log_error("shell renderer could not close the drawing process");
        return;
    }

    const uint64_t line_height = 16U;
    const uint64_t top_margin = 8U;
    for (size_t row = 0U; row < SHELL_OUTPUT_LINES; row++) {
        if (shell_screen[row].dirty) {
            point_t pos = {.x = 8, .y = (int64_t)(top_margin + ((uint64_t)row * line_height))};
            log_debug("text: %s", (const char *)(shell_screen->rows[row].chars));
            (void)canvas_draw_text(canvas, pos, (const char *)(shell_screen->rows[row].chars), shell_foreground, 1U);
        }
    }

    (void)layer_draw_end(layer);
}

void shell_render_thread(void *arg) {
    (void)arg;
    while (shell_is_running()) {
        shell_render_frame();
        log_trace("render");
        thread_sleep_ms(16U);
    }
    log_debug("exit");
    thread_exit();
}
