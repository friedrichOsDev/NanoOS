/**
 * @file screen.c
 * @brief Character-cell screen model and UTF-8 text placement.
 * @author friedrichOsDev
 */

#include <lib/util/mem.h>
#include <shell/screen.h>

#include <mm/memory.h>

/**
 * @brief Decode one UTF-8 codepoint.
 *
 * Invalid sequences are consumed as U+FFFD to keep the screen model valid.
 *
 * @param[in,out] input Current input pointer.
 * @return Decoded Unicode codepoint.
 */
static uint32_t screen_decode_utf8(const uint8_t **input) {
    const uint8_t *s = *input;
    uint32_t cp;
    size_t length;

    if (s[0] < 0x80U) {
        *input = s + 1;
        return s[0];
    }
    if ((s[0] & 0xE0U) == 0xC0U) {
        cp = (uint32_t)(s[0] & 0x1FU);
        length = 2U;
    } else if ((s[0] & 0xF0U) == 0xE0U) {
        cp = (uint32_t)(s[0] & 0x0FU);
        length = 3U;
    } else if ((s[0] & 0xF8U) == 0xF0U) {
        cp = (uint32_t)(s[0] & 0x07U);
        length = 4U;
    } else {
        *input = s + 1;
        return 0xFFFDU;
    }

    for (size_t i = 1U; i < length; i++) {
        if ((s[i] & 0xC0U) != 0x80U) {
            *input = s + 1;
            return 0xFFFDU;
        }
        cp = (cp << 6) | (uint32_t)(s[i] & 0x3FU);
    }

    if ((length == 2U && cp < 0x80U) ||
        (length == 3U && cp < 0x800U) ||
        (length == 4U && cp < 0x10000U) ||
        cp > 0x10FFFFU || (cp >= 0xD800U && cp <= 0xDFFFU)) {
        *input = s + 1;
        return 0xFFFDU;
    }

    *input = s + length;
    return cp;
}

/**
 * @brief Initialize a character-cell screen.
 *
 * @param[out] screen Screen model to initialize.
 * @param[in] columns Number of character columns.
 * @param[in] rows Number of character rows.
 * @return E_OK on success or an error code.
 */
kerr_t shell_screen_init(shell_screen_t *screen, size_t columns, size_t rows) {
    if (!screen || columns == 0U || rows == 0U || columns > SIZE_MAX / rows) {
        return E_INVAL;
    }

    size_t count = columns * rows;
    if (count > SIZE_MAX / sizeof(uint32_t)) {
        return E_INVAL;
    }

    virt_addr_t memory = 0;
    K_TRY(kzalloc(count * sizeof(uint32_t), &memory));

    screen->columns = columns;
    screen->rows = rows;
    screen->cells = (uint32_t *)memory;
    return E_OK;
}

/**
 * @brief Clear all cells to spaces.
 *
 * @param[in,out] screen Screen model.
 * @return E_OK on success or E_INVAL for an invalid screen.
 */
kerr_t shell_screen_clear(shell_screen_t *screen) {
    if (!screen || !screen->cells || screen->columns == 0U || screen->rows == 0U) {
        return E_INVAL;
    }

    memset32(screen->cells, 0U, screen->columns * screen->rows);
    return E_OK;
}

/**
 * @brief Write UTF-8 text into the screen model, wrapping at the right edge.
 *
 * Newline moves to the first column of the next row. Text outside the screen
 * is clipped. This function only updates character data; it does not render.
 *
 * @param[in,out] screen Target screen model.
 * @param[in] column Starting column.
 * @param[in] row Starting row.
 * @param[in] text UTF-8 text to place.
 * @return E_OK on success or E_INVAL for invalid arguments.
 */
kerr_t shell_screen_write(shell_screen_t *screen, size_t column, size_t row, const char *text) {
    if (!screen || !screen->cells || !text ||
        screen->columns == 0U || screen->rows == 0U ||
        column >= screen->columns || row >= screen->rows) {
        return E_INVAL;
    }

    const uint8_t *input = (const uint8_t *)text;
    while (*input != 0U && row < screen->rows) {
        uint32_t cp = screen_decode_utf8(&input);
        if (cp == (uint32_t)'\n') {
            column = 0U;
            row++;
            continue;
        }
        if (cp == (uint32_t)'\r') {
            column = 0U;
            continue;
        }
        if (cp == (uint32_t)'\t') {
            size_t spaces = 4U - (column % 4U);
            while (spaces > 0U && row < screen->rows) {
                screen->cells[(row * screen->columns) + column] = (uint32_t)' ';
                spaces--;
                column++;
                if (column >= screen->columns) {
                    column = 0U;
                    row++;
                }
            }
            continue;
        }

        screen->cells[(row * screen->columns) + column] = cp;
        column++;
        if (column >= screen->columns) {
            column = 0U;
            row++;
        }
    }

    return E_OK;
}

/**
 * @brief Release the screen cell buffer.
 *
 * @param[in,out] screen Screen model to destroy.
 * @return E_OK on success or E_INVAL for a NULL model.
 */
kerr_t shell_screen_destroy(shell_screen_t *screen) {
    if (!screen) {
        return E_INVAL;
    }
    if (screen->cells) {
        K_TRY(kfree((virt_addr_t)screen->cells));
    }
    screen->cells = NULL;
    screen->columns = 0U;
    screen->rows = 0U;
    return E_OK;
}
