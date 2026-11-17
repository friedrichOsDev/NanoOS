/**
 * @file _shell.h
 * @brief Internal shell header.
 * @author friedrichOsDev
 */

#pragma once

#include <drivers/gfx/compositor.h>
#include <pch.h>

#define SHELL_INPUT_BUFFER_SIZE 1024

/** @brief Logical shell input representation (all logic happens on this buffer, the renderer draws without knowing something about the insertions, etc) */
typedef struct {
    uint32_t buffer[SHELL_INPUT_BUFFER_SIZE]; /**< Buffer holding the input */
    uint64_t size;                            /**< Current size of the buffer */
    uint64_t cursor_pos;                      /**< Current position in the buffer (0: crusor is before the 1st element, 1: cursor is after the 1st element, etc. ) */
} shell_input_buffer_t;

extern shell_input_buffer_t shell_input_buffer; // Global shell input buffer

kerr_t shell_input_init(void);
kerr_t shell_input_deinit(void);
bool shell_sync_input_buffer(shell_input_buffer_t *out_buffer);

/** @brief Shell cell representation (used for rendering the shell screen)  */
typedef struct {
    uint32_t unicode; /**< Unicode code point of the cell */
    color_t fg;       /**< Foreground color of the cell */
    color_t bg;       /**< Background color of the cell */
} shell_cell_t;

/** @brief Virtual Screen representation (the renderer draws the shell screen every frame) */
typedef struct {
    uint8_t font_scale;           /**< The font scale (between 1U and 255U, but who would ever use 255U ;) ) */
    uint64_t lines;               /**< Vertical line count (screen height / font height at the scale) */
    uint64_t columns;             /**< Horizontal column count (screen width / font width at the scale) */
    uint64_t buffer_size;         /**< The screen buffer size (lines * columns) */
    shell_cell_t *buffer;         /**< Heap allocated screen buffer (buffer size * sizeof(shell_cell_t)) */
    bool is_dirty;                /**< Global fast-check flag: true if at least one line changed */
    bool *dirty_lines;            /**< Heap allocated array of size (lines * sizeof(bool)) */
    uint64_t pixel_width;         /**< Current screen width in pixels */
    uint64_t pixel_height;        /**< Current screen height in pixels */
    uint64_t glyph_width;         /**< Unscaled glyph width in pixels */
    uint64_t glyph_height;        /**< Unscaled glyph height in pixels */
    uint64_t input_scroll;        /**< First input codepoint currently visible */
    uint64_t input_visible_count; /**< Number of visible input codepoints */
    uint64_t prompt_columns;      /**< Number of prompt cells currently visible */
    uint64_t cursor_column;       /**< Cursor column in the screen grid */
    uint64_t cursor_line;         /**< Cursor line in the screen grid */
    bool cursor_visible;          /**< Whether the cursor is inside the visible grid */
} shell_screen_t;

extern bool shell_initialized;

kerr_t shell_init_renderer(void);
kerr_t shell_deinit_renderer(void);
void shell_render_thread(void *arg);
