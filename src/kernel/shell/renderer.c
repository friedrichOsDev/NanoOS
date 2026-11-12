/**
 * @file renderer.c
 * @brief Render changed character cells through the shell compositor layer.
 * @author friedrichOsDev
 */

#include <drivers/gfx/compositor.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <shell/renderer.h>

#define SHELL_FONT_WIDTH 8U
#define SHELL_FONT_HEIGHT 16U
#define SHELL_RENDER_LAYER_Z 1000

static layer_t *shell_layer;
static char *row_utf8;
static uint32_t *previous_cells;
static size_t row_buffer_size;
static size_t renderer_columns;
static size_t renderer_rows;
static bool renderer_cache_valid;
static bool renderer_initialized;

/**
 * @brief Encode one Unicode scalar value as UTF-8.
 *
 * @param[in] codepoint Unicode codepoint.
 * @param[out] output Buffer with at least five bytes.
 * @return Number of encoded bytes.
 */
static size_t renderer_encode_utf8(uint32_t codepoint, char output[5]) {
    if (codepoint > 0x10FFFFU || (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
        codepoint = 0xFFFDU;
    }

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

    output[0] = (char)(0xF0U | (codepoint >> 18));
    output[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
    output[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
    output[3] = (char)(0x80U | (codepoint & 0x3FU));
    return 4U;
}

/**
 * @brief Return the character currently visible at a screen cell.
 *
 * @param[in] screen Character-cell screen.
 * @param[in] index Linear cell index.
 * @param[in] cursor_index Cursor overlay position.
 * @return Visible Unicode codepoint.
 */
static uint32_t renderer_visible_cell(const shell_screen_t *screen,
                                      size_t index,
                                      size_t cursor_index) {
    if (index == cursor_index) {
        return (uint32_t)'_';
    }

    uint32_t codepoint = screen->cells[index];
    return (codepoint == 0U) ? (uint32_t)' ' : codepoint;
}

/**
 * @brief Draw one changed horizontal run of cells.
 *
 * The old pixels are cleared before drawing so spaces and deleted characters
 * correctly erase their previous glyphs.
 *
 * @param[in] canvas Target shell layer canvas.
 * @param[in] screen Current screen model.
 * @param[in] row Row to update.
 * @param[in] first_column First column in the run.
 * @param[in] column_count Number of cells in the run.
 * @param[in] cursor_index Cursor overlay position.
 * @return E_OK on success or an error code.
 */
static kerr_t renderer_draw_run(canvas_t *canvas,
                                const shell_screen_t *screen,
                                size_t row,
                                size_t first_column,
                                size_t column_count,
                                size_t cursor_index) {
    size_t offset = 0U;
    size_t row_start = row * screen->columns;

    for (size_t column = 0U; column < column_count; column++) {
        size_t index = row_start + first_column + column;
        uint32_t codepoint = renderer_visible_cell(screen, index, cursor_index);
        char encoded[5] = {0};
        size_t encoded_length = renderer_encode_utf8(codepoint, encoded);

        if (offset + encoded_length >= row_buffer_size) {
            return E_BADSIZE;
        }

        for (size_t i = 0U; i < encoded_length; i++) {
            row_utf8[offset++] = encoded[i];
        }
    }
    row_utf8[offset] = '\0';

    point_t position = {
        .x = (int64_t)(first_column * SHELL_FONT_WIDTH),
        .y = (int64_t)(row * SHELL_FONT_HEIGHT)};
    rect_size_t run_size = {
        .width = column_count * SHELL_FONT_WIDTH,
        .height = SHELL_FONT_HEIGHT};

    K_TRY(canvas_draw_rectangle(canvas, position, run_size,
                                COLOR_BLACK, true, 0U));
    return canvas_draw_text(canvas, position, row_utf8,
                            MAKE_COLOR(224, 228, 234), 1U);
}

/**
 * @brief Initialize the fullscreen shell layer and its diff buffers.
 *
 * @return E_OK on success or an error code.
 */
kerr_t shell_renderer_init(void) {
    if (renderer_initialized) {
        return E_OK;
    }

    rect_size_t size = compositor_get_size();
    if (size.width < SHELL_FONT_WIDTH || size.height < SHELL_FONT_HEIGHT) {
        return E_NOTAVAIL;
    }

    renderer_columns = (size_t)(size.width / SHELL_FONT_WIDTH);
    renderer_rows = (size_t)(size.height / SHELL_FONT_HEIGHT);
    if (renderer_columns > (SIZE_MAX - 1U) / 4U ||
        renderer_columns > SIZE_MAX / renderer_rows ||
        renderer_columns * renderer_rows > SIZE_MAX / sizeof(uint32_t)) {
        return E_INVAL;
    }

    size_t cell_count = renderer_columns * renderer_rows;
    row_buffer_size = (renderer_columns * 4U) + 1U;

    virt_addr_t memory = 0U;
    kerr_t err = kzalloc(row_buffer_size, &memory);
    if (k_failed(err)) {
        goto fail;
    }
    row_utf8 = (char *)memory;

    memory = 0U;
    err = kzalloc(cell_count * sizeof(uint32_t), &memory);
    if (k_failed(err)) {
        goto fail;
    }
    previous_cells = (uint32_t *)memory;

    err = layer_create(size, SHELL_RENDER_LAYER_Z, &shell_layer);
    if (k_failed(err)) {
        goto fail;
    }

    renderer_cache_valid = false;
    renderer_initialized = true;
    return E_OK;

fail:
    if (previous_cells) {
        (void)kfree((virt_addr_t)previous_cells);
        previous_cells = NULL;
    }
    if (row_utf8) {
        (void)kfree((virt_addr_t)row_utf8);
        row_utf8 = NULL;
    }
    row_buffer_size = 0U;
    renderer_columns = 0U;
    renderer_rows = 0U;
    return err;
}

/**
 * @brief Update only cells whose visible character differs from the last frame.
 *
 * The screen is scanned in memory, but graphics operations are issued only for
 * contiguous changed runs. The cursor is a renderer overlay and is not stored
 * in the caller-owned screen model.
 *
 * @param[in] screen Character-cell screen model.
 * @param[in] cursor_index Linear cursor position.
 * @return E_OK on success or an error code.
 */
kerr_t shell_renderer_render(const shell_screen_t *screen, size_t cursor_index) {
    size_t cell_count = renderer_columns * renderer_rows;

    if (!renderer_initialized || !shell_layer || !screen || !screen->cells ||
        screen->columns != renderer_columns || screen->rows != renderer_rows ||
        (cursor_index != (size_t)-1 && cursor_index >= cell_count)) {
        return E_INVAL;
    }

    K_TRY(layer_draw_begin(shell_layer));
    canvas_t *canvas = &shell_layer->canvas;
    kerr_t err = E_OK;

    if (!renderer_cache_valid) {
        err = canvas_clear(canvas, COLOR_BLACK);
    }

    for (size_t row = 0U; row < renderer_rows && !k_failed(err); row++) {
        size_t column = 0U;
        size_t row_start = row * renderer_columns;

        while (column < renderer_columns && !k_failed(err)) {
            size_t index = row_start + column;
            uint32_t visible = renderer_visible_cell(screen, index, cursor_index);
            bool changed = !renderer_cache_valid ||
                           visible != previous_cells[index];

            if (!changed) {
                column++;
                continue;
            }

            size_t run_start = column;
            do {
                index = row_start + column;
                visible = renderer_visible_cell(screen, index, cursor_index);
                changed = !renderer_cache_valid ||
                          visible != previous_cells[index];
                if (!changed) {
                    break;
                }
                column++;
            } while (column < renderer_columns);

            size_t run_length = column - run_start;
            err = renderer_draw_run(canvas, screen, row, run_start,
                                    run_length, cursor_index);
            if (!k_failed(err)) {
                for (size_t i = 0U; i < run_length; i++) {
                    size_t changed_index = row_start + run_start + i;
                    previous_cells[changed_index] =
                        renderer_visible_cell(screen, changed_index, cursor_index);
                }
            }
        }
    }

    if (!k_failed(err)) {
        renderer_cache_valid = true;
    } else {
        /* Force a complete repaint next time after any partial failure. */
        renderer_cache_valid = false;
    }

    kerr_t end_err = layer_draw_end(shell_layer);
    if (k_failed(err)) {
        return err;
    }
    return end_err;
}

/**
 * @brief Destroy the shell layer and all renderer-owned buffers.
 *
 * @return E_OK on success or an error code.
 */
kerr_t shell_renderer_deinit(void) {
    if (shell_layer) {
        K_TRY(layer_destroy(shell_layer));
        shell_layer = NULL;
    }
    if (previous_cells) {
        K_TRY(kfree((virt_addr_t)previous_cells));
        previous_cells = NULL;
    }
    if (row_utf8) {
        K_TRY(kfree((virt_addr_t)row_utf8));
        row_utf8 = NULL;
    }

    row_buffer_size = 0U;
    renderer_columns = 0U;
    renderer_rows = 0U;
    renderer_cache_valid = false;
    renderer_initialized = false;
    return E_OK;
}
