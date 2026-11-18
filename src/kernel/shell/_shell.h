/**
 * @file _shell.h
 * @brief Internal shell header.
 * @author friedrichOsDev
 */
#pragma once
#include <drivers/gfx/compositor.h>
#include <pch.h>

#define SHELL_INPUT_BUFFER_SIZE 1024
typedef struct {
    uint32_t buffer[SHELL_INPUT_BUFFER_SIZE];
    uint64_t size;
    uint64_t cursor_pos;
} shell_input_buffer_t;

extern shell_input_buffer_t shell_input_buffer;
kerr_t shell_input_init();
kerr_t shell_input_deinit();
bool shell_sync_input_buffer(shell_input_buffer_t *out_buffer);

typedef struct {
    uint32_t unicode;
    color_t fg;
    color_t bg;
} shell_cell_t;

typedef struct {
    uint8_t font_scale;
    uint64_t lines;
    uint64_t columns;
    uint64_t buffer_size;
    shell_cell_t *buffer;
    bool is_dirty;
    bool *dirty_lines;
    uint64_t pixel_width;
    uint64_t pixel_height;
    uint64_t glyph_width;
    uint64_t glyph_height;
    uint64_t input_scroll;
    uint64_t input_visible_count;
    uint64_t prompt_columns;
    uint64_t cursor_column;
    uint64_t cursor_line;
    bool cursor_visible;
} shell_screen_t;

extern bool shell_initialized;
kerr_t shell_init_renderer();
kerr_t shell_deinit_renderer();
bool shell_renderer_is_stopped(void);
void shell_render_thread(void *arg);
