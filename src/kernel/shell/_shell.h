/**
 * @file _shell.h
 * @brief Internal shell header.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define SHELL_INPUT_BUFFER_SIZE 1024

/** @brief Logical shell input representation (all logic happens on this buffer, the renderer draws without knowing something about the insertions, etc) */
typedef struct {
    uint32_t buffer[SHELL_INPUT_BUFFER_SIZE]; /**< Buffer holding the input */
    uint64_t size;                            /**< Current size of the buffer */
    uint64_t cursor_pos;                      /**< Current position in the buffer (0: crusor is before the 1st element, 1: cursor is after the 1st element, etc. ) */
} shell_input_buffer_t;

extern shell_input_buffer_t shell_input_buffer;  // Global shell input buffer

kerr_t shell_input_init();

extern bool shell_initialized;
