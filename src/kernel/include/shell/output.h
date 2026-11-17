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

/**
 * @brief Initializes the shell output buffer.
 *
 * The buffer uses fixed static storage and does not allocate from the heap.
 *
 * @return E_OK on success, or an error code.
 */
kerr_t shell_output_init(void);

/**
 * @brief Disables and clears the shell output buffer.
 *
 * @return E_OK on success.
 */
kerr_t shell_output_deinit(void);

/**
 * @brief Writes a null-terminated UTF-8 string to the output buffer.
 *
 * Newline and tab characters are interpreted; invalid UTF-8 is replaced with
 * U+FFFD. A null pointer returns E_BADADDR.
 *
 * @param[in] text Null-terminated UTF-8 text.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_write(const char *text);

/**
 * @brief Writes exactly length bytes of UTF-8 text to the output buffer.
 *
 * @param[in] text UTF-8 input bytes.
 * @param[in] length Number of bytes to consume.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_write_n(const char *text, size_t length);

/**
 * @brief Writes one Unicode codepoint to the output buffer.
 *
 * Newline and tab codepoints receive the same treatment as in shell_write().
 *
 * @param[in] codepoint Unicode codepoint to append.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_write_u32(uint32_t codepoint);

/**
 * @brief Writes a UTF-8 string followed by a newline.
 *
 * @param[in] text Null-terminated UTF-8 text.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_write_line(const char *text);

/**
 * @brief Clears all stored output and leaves one empty current line.
 *
 * @return E_OK on success, or an error code.
 */
kerr_t shell_clear_output(void);

/**
 * @brief Copies a consistent output snapshot if its generation has changed.
 *
 * The newest lines are copied when capacity is smaller than the stored line
 * count. Pass the previous generation in *inout_generation; it is updated
 * after a successful snapshot. A zero capacity is valid and copies no lines.
 *
 * @param[out] lines Destination array, unless capacity is zero.
 * @param[in] capacity Number of entries available in lines.
 * @param[out] out_count Number of copied lines.
 * @param[in,out] inout_generation Previous generation; updated on change.
 * @param[out] out_changed Whether a new snapshot was copied.
 * @return E_OK on success, E_BADADDR for invalid pointers, or E_NOINIT.
 */
kerr_t shell_output_snapshot(
    shell_output_line_t *lines,
    size_t capacity,
    size_t *out_count,
    uint64_t *inout_generation,
    bool *out_changed);

/**
 * @brief Runs a small destructive self-test of the shell output API.
 *
 * The test replaces the current output with three demonstration lines.
 *
 * @return E_OK when UTF-8, line handling, and snapshots pass; otherwise an
 *         error code.
 */
kerr_t shell_output_self_test(void);
