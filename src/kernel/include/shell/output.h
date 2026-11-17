/**
 * @file output.h
 * @brief Public shell output buffer interface.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define SHELL_OUTPUT_MAX_LINES 128U
#define SHELL_OUTPUT_MAX_LINE_LENGTH 256U

/** @brief One logical line in a shell output snapshot. */
typedef struct {
    uint32_t codepoints[SHELL_OUTPUT_MAX_LINE_LENGTH]; /**< Unicode codepoints in this line. */
    uint16_t length;                                   /**< Number of valid codepoints. */
} shell_output_line_t;

kerr_t shell_output_init();
kerr_t shell_output_deinit();
kerr_t shell_write(const char *text);
kerr_t shell_write_n(const char *text, size_t length);
kerr_t shell_write_u32(uint32_t codepoint);
kerr_t shell_write_line(const char *text);
kerr_t shell_clear_output();
kerr_t shell_output_snapshot(shell_output_line_t *lines, size_t capacity, size_t *out_count, uint64_t *inout_generation, bool *out_changed);
