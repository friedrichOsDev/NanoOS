/**
 * @file shell_renderer.c
 * @brief Fullscreen kernel shell rendering subsystem.
 * @author friedrichOsDev
 */

#include "shell_renderer.h"
#include "shell_internal.h"
#include <drivers/gfx/compositor.h>
#include <lib/util/str.h>
#include <sched/sched.h>
#include <shell/shell.h>

static size_t encode_codepoint(uint32_t codepoint, char *output) {
    if (codepoint <= 0x7FU) {
        output[0] = (char)codepoint;
        return 1U;
    }
    if (codepoint <= 0x7FFU) {
        output[0] = (char)(0xC0U | (codepoint >> 6));
        output[1] = (char)(0x80U | (codepoint & 0x3FU));
        return 2U;
    }
    if (codepoint <= 0xFFFFU) {
        output[0] = (char)(0xE0U | (codepoint >> 12));
        output[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        output[2] = (char)(0x80U | (codepoint & 0x3FU));
        return 3U;
    }
    if (codepoint <= 0x10FFFFU) {
        output[0] = (char)(0xF0U | (codepoint >> 18));
        output[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
        output[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        output[3] = (char)(0x80U | (codepoint & 0x3FU));
        return 4U;
    }
    output[0] = '?';
    return 1U;
}

/**
 * @brief Draws the current terminal state into the shell layer.
 */
static void shell_render_frame(void) {
    layer_t *layer = shell_get_layer();
    if (!layer || k_failed(layer_draw_begin(layer))) {
        return;
    }

    irq_flags_t flags = shell_screen_lock();
    shell_screen_t *screen = shell_get_screen();
    if (!screen->dirty) {
        shell_screen_unlock(flags);
        (void)layer_draw_end(layer);
        return;
    }

    canvas_t *canvas = &layer->canvas;
    const color_t foreground = MAKE_COLOR(224, 228, 232);
    (void)canvas_clear(canvas, COLOR_BLACK);

    const int64_t left_margin = 8;
    const int64_t top_margin = 8;
    const int64_t line_height = 16;
    const int64_t glyph_width = 8;

    for (size_t row = 0U; row < screen->line_count; row++) {
        point_t position = {
            .x = left_margin,
            .y = top_margin + ((int64_t)row * line_height)};
        (void)canvas_draw_text(canvas, position, screen->rows[row].text, foreground, 1U);
    }

    size_t prompt_length = strlen(SHELL_PROMPT);
    int64_t prompt_x = left_margin;
    int64_t prompt_y = top_margin + ((int64_t)screen->line_count * line_height);
    (void)canvas_draw_text(canvas, (point_t){.x = prompt_x, .y = prompt_y},
                           SHELL_PROMPT, foreground, 1U);

    char input_text[(SHELL_INPUT_MAX * 4U) + 1U];
    size_t byte_offsets[SHELL_INPUT_MAX + 1U];
    size_t byte_length = 0U;
    for (size_t i = 0U; i < screen->input_length; i++) {
        byte_offsets[i] = byte_length;
        byte_length += encode_codepoint(screen->input[i], &input_text[byte_length]);
    }
    byte_offsets[screen->input_length] = byte_length;
    input_text[byte_length] = '\0';

    size_t available_columns = (screen->columns > prompt_length)
                                   ? screen->columns - prompt_length
                                   : 0U;
    size_t start_index = 0U;
    if (available_columns > 0U && screen->input_cursor >= available_columns) {
        start_index = screen->input_cursor - available_columns + 1U;
    }
    size_t end_index = start_index + available_columns;
    if (end_index > screen->input_length) {
        end_index = screen->input_length;
    }

    size_t start_byte = byte_offsets[start_index];
    size_t cursor_byte = byte_offsets[screen->input_cursor];
    size_t end_byte = byte_offsets[end_index];

    char end_saved = input_text[end_byte];
    input_text[end_byte] = '\0';
    char cursor_saved = input_text[cursor_byte];
    input_text[cursor_byte] = '\0';

    int64_t input_x = left_margin + ((int64_t)prompt_length * glyph_width);
    (void)canvas_draw_text(canvas, (point_t){.x = input_x, .y = prompt_y},
                           input_text + start_byte, foreground, 1U);

    int64_t cursor_x = input_x +
                       ((int64_t)(screen->input_cursor - start_index) * glyph_width);
    (void)canvas_draw_rectangle(canvas,
                                (point_t){.x = cursor_x, .y = prompt_y},
                                (rect_size_t){.width = 8U, .height = 16U},
                                foreground, true, 0U);

    input_text[cursor_byte] = cursor_saved;
    (void)canvas_draw_text(canvas,
                           (point_t){.x = cursor_x + glyph_width, .y = prompt_y},
                           input_text + cursor_byte, foreground, 1U);
    input_text[end_byte] = end_saved;

    screen->dirty = false;
    shell_screen_unlock(flags);
    (void)layer_draw_end(layer);
}

/**
 * @brief Kernel thread that continuously refreshes the shell layer.
 * @param[in] arg Unused thread argument.
 */
void shell_render_thread(void *arg) {
    (void)arg;

    while (shell_is_running()) {
        shell_render_frame();
        thread_sleep_ms(16U);
    }

    shell_renderer_finished();
    thread_exit();
}
