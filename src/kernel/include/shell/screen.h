/**
 * @file screen.h
 * @brief Character-cell screen model for the kernel shell.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief In-memory terminal contents, independent from the graphics backend. */
typedef struct {
    size_t columns;     /**< Number of character columns. */
    size_t rows;        /**< Number of character rows. */
    uint32_t *cells;    /**< Row-major Unicode codepoints; zero represents a space. */
} shell_screen_t;

kerr_t shell_screen_init(shell_screen_t *screen, size_t columns, size_t rows);
kerr_t shell_screen_clear(shell_screen_t *screen);
kerr_t shell_screen_write(shell_screen_t *screen, size_t column, size_t row, const char *text);
kerr_t shell_screen_destroy(shell_screen_t *screen);
