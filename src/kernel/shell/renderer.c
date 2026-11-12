/**
 * @file renderer.c
 * @brief Render a character-cell shell screen through the compositor.
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
static size_t row_buffer_size;
static size_t renderer_columns;
static size_t renderer_rows;
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
 * @brief Initialize the fullscreen shell rendering layer and row conversion buffer.
 *
 * @return E_OK on success or an error code if graphics are unavailable.
 */
kerr_t shell_renderer_init() {
    if (renderer_initialized) {
        return E_OK;
    }

    rect_size_t size = compositor_get_size();
    if (size.width < SHELL_FONT_WIDTH || size.height < SHELL_FONT_HEIGHT) {
        return E_NOTAVAIL;
    }

    renderer_columns = (size_t)(size.width / SHELL_FONT_WIDTH);
    renderer_rows = (size_t)(size.height / SHELL_FONT_HEIGHT);
    if (renderer_columns > (SIZE_MAX - 1U) / 4U) {
        return E_INVAL;
    }

    row_buffer_size = (renderer_columns * 4U) + 1U;
    virt_addr_t memory = 0;
    K_TRY(kzalloc(row_buffer_size, &memory));
    row_utf8 = (char *)memory;

    kerr_t err = layer_create(size, SHELL_RENDER_LAYER_Z, &shell_layer);
    if (k_failed(err)) {
        kfree((virt_addr_t)row_utf8);
        row_utf8 = NULL;
        row_buffer_size = 0U;
        return err;
    }

    renderer_initialized = true;
    return E_OK;
}

/**
 * @brief Render the supplied screen model as a fullscreen terminal.
 *
 * The renderer is deliberately stateless with respect to terminal contents:
 * every visible character comes from the caller-owned screen data structure.
 *
 * @param[in] screen Character-cell screen model.
 * @return E_OK on success or an error code.
 */
kerr_t shell_renderer_render(const shell_screen_t *screen) {
    if (!renderer_initialized || !shell_layer || !screen || !screen->cells ||
        screen->columns != renderer_columns || screen->rows != renderer_rows) {
        return E_INVAL;
    }

    K_TRY(layer_draw_begin(shell_layer));
    canvas_t *canvas = &shell_layer->canvas;
    kerr_t err = canvas_clear(canvas, COLOR_BLACK);
    const color_t foreground = MAKE_COLOR(224, 228, 234);

    for (size_t row = 0U; row < screen->rows && !k_failed(err); row++) {
        size_t offset = 0U;
        for (size_t column = 0U; column < screen->columns; column++) {
            uint32_t codepoint = screen->cells[(row * screen->columns) + column];
            if (codepoint == 0U) {
                codepoint = (uint32_t)' ';
            }

            char encoded[5] = {0};
            size_t encoded_length = renderer_encode_utf8(codepoint, encoded);
            if (offset + encoded_length >= row_buffer_size) {
                err = E_BADSIZE;
                break;
            }
            for (size_t i = 0U; i < encoded_length; i++) {
                row_utf8[offset++] = encoded[i];
            }
        }

        if (!k_failed(err)) {
            row_utf8[offset] = '\0';
            point_t position = {
                .x = 0,
                .y = (int64_t)(row * SHELL_FONT_HEIGHT)};
            err = canvas_draw_text(canvas, position, row_utf8, foreground, 1U);
        }
    }

    kerr_t end_err = layer_draw_end(shell_layer);
    if (k_failed(err)) {
        return err;
    }
    return end_err;
}

/**
 * @brief Destroy the shell rendering layer and conversion buffer.
 *
 * @return E_OK on success or an error code.
 */
kerr_t shell_renderer_deinit() {
    if (shell_layer) {
        K_TRY(layer_destroy(shell_layer));
        shell_layer = NULL;
    }
    if (row_utf8) {
        K_TRY(kfree((virt_addr_t)row_utf8));
        row_utf8 = NULL;
    }

    row_buffer_size = 0U;
    renderer_columns = 0U;
    renderer_rows = 0U;
    renderer_initialized = false;
    return E_OK;
}
