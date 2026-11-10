/**
 * @file shell_renderer.c
 * @brief Fullscreen NanoOS shell renderer.
 * @author friedrichOsDev
 */

#include "shell_renderer.h"
#include "shell_internal.h"
#include <drivers/gfx/compositor.h>
#include <sched/sched.h>
#include <shell/shell.h>

static const color_t shell_background = MAKE_COLOR(0, 0, 0);
static const color_t shell_foreground = MAKE_COLOR(224, 228, 232);
static const color_t shell_cursor_color = MAKE_COLOR(224, 228, 232);

/** @brief Counts UTF-8 leading bytes before the supplied byte offset. */
static size_t utf8_character_count(const char *text, size_t bytes) {
    size_t count = 0U;
    for (size_t i = 0U; i < bytes; i++) {
        if (((uint8_t)text[i] & 0xC0U) != 0x80U) count++;
    }
    return count;
}

/** @brief Advances to the byte offset of the requested UTF-8 character index. */
static size_t utf8_byte_offset(const char *text, size_t bytes, size_t characters) {
    size_t count = 0U;
    size_t offset = 0U;
    while (offset < bytes && count < characters) {
        if (((uint8_t)text[offset] & 0xC0U) != 0x80U) count++;
        offset++;
    }
    while (offset < bytes && (((uint8_t)text[offset] & 0xC0U) == 0x80U)) offset++;
    return offset;
}

/** @brief Copies shell state and draws a complete terminal frame. */
void shell_render_frame(void) {
    static char lines[SHELL_OUTPUT_LINES][SHELL_OUTPUT_WIDTH];
    static char input[SHELL_INPUT_MAX];
    size_t line_count = 0U;
    size_t cursor_byte = 0U;
    size_t input_bytes = 0U;

    shell_get_snapshot(lines, &line_count, input, &input_bytes, &cursor_byte);
    layer_t *layer = shell_get_layer();
    if (!layer) return;

    rect_size_t size = layer->canvas.size;
    if (size.width == 0U || size.height < 48U ||
        size.width > (uint64_t)INT64_MAX || size.height > (uint64_t)INT64_MAX) {
        return;
    }

    if (k_failed(layer_draw_begin(layer))) return;
    canvas_t *canvas = &layer->canvas;
    canvas_clear(canvas, shell_background);

    const uint64_t line_height = 16U;
    const uint64_t top_margin = 8U;
    const uint64_t prompt_y = size.height - 24U;
    uint64_t visible_rows = (prompt_y > top_margin) ? ((prompt_y - top_margin) / line_height) : 0U;
    if (visible_rows > line_count) visible_rows = line_count;
    size_t first_row = line_count - (size_t)visible_rows;

    for (size_t row = 0U; row < (size_t)visible_rows; row++) {
        point_t pos = {.x = 8, .y = (int64_t)(top_margin + ((uint64_t)row * line_height))};
        (void)canvas_draw_text(canvas, pos, lines[first_row + row], shell_foreground, 1U);
    }

    const char *prompt = "friedrichOsDev@nanoos> ";
    const size_t prompt_chars = 23U;
    point_t prompt_pos = {.x = 8, .y = (int64_t)prompt_y};
    (void)canvas_draw_text(canvas, prompt_pos, prompt, shell_foreground, 1U);

    size_t cursor_chars = utf8_character_count(input, cursor_byte);
    size_t max_chars = (size.width > 24U) ? (size_t)((size.width - 24U) / 8U) : 1U;
    max_chars = (max_chars > prompt_chars) ? max_chars - prompt_chars : 1U;
    size_t first_char = (cursor_chars >= max_chars) ? cursor_chars - max_chars + 1U : 0U;
    size_t first_byte = utf8_byte_offset(input, input_bytes, first_char);

    point_t input_pos = {
        .x = 8 + (int64_t)(prompt_chars * 8U),
        .y = (int64_t)prompt_y};
    (void)canvas_draw_text(canvas, input_pos, input + first_byte, shell_foreground, 1U);

    uint64_t cursor_x = 8U + ((uint64_t)prompt_chars * 8U) +
                        ((uint64_t)(cursor_chars - first_char) * 8U);
    if (cursor_x + 2U < size.width) {
        (void)canvas_draw_rectangle(canvas,
                                    (point_t){.x = (int64_t)cursor_x, .y = (int64_t)prompt_y},
                                    (rect_size_t){.width = 2U, .height = 16U},
                                    shell_cursor_color, true, 0U);
    }

    (void)layer_draw_end(layer);
}

/**
 * @brief Periodically redraws the terminal until the shell is stopped.
 * @param[in] arg Unused thread argument.
 */
void shell_render_thread(void *arg) {
    (void)arg;
    while (shell_is_running()) {
        shell_render_frame();
        thread_sleep_ms(16U);
    }
    thread_exit();
}
