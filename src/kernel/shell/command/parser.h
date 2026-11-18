/**
 * @file parser.h
 * @brief Internal shell command parser interface.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define SHELL_COMMAND_MAX_ARGS 16U
#define SHELL_COMMAND_MAX_LINE_CODEPOINTS 1024U
#define SHELL_COMMAND_PARSE_STORAGE_SIZE \
    ((SHELL_COMMAND_MAX_LINE_CODEPOINTS * 4U) + SHELL_COMMAND_MAX_ARGS)

/** @brief Parser result with all argument strings stored in one fixed buffer. */
typedef struct {
    char storage[SHELL_COMMAND_PARSE_STORAGE_SIZE];
    const char *argv[SHELL_COMMAND_MAX_ARGS];
    size_t argc;
    size_t storage_used;
} shell_parse_result_t;

/**
 * @brief Parse Unicode codepoints into a null-terminated UTF-8 argv vector.
 *
 * @param[in] input Input codepoints.
 * @param[in] length Number of input codepoints.
 * @param[out] result Fixed parser workspace.
 * @return E_OK on success, or an error code for invalid syntax/limits.
 */
kerr_t shell_parse(const uint32_t *input, size_t length, shell_parse_result_t *result);
