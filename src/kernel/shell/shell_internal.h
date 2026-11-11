/**
 * @file shell_internal.h
 * @brief Private shell interfaces shared by shell modules.
 * @author friedrichOsDev
 */

#pragma once

#include <drivers/gfx/compositor.h>
#include <drivers/hid/input.h>
#include <pch.h>

#define SHELL_INPUT_MAX 256U
// #define SHELL_HISTORY_COUNT 8U
#define SHELL_OUTPUT_LINES 64U
#define SHELL_OUTPUT_WIDTH 160U

typedef struct {
    uint32_t c;
} shell_char_t;

typedef struct {
    shell_char_t chars[SHELL_OUTPUT_WIDTH];
    bool dirty;
} shell_row_t;

typedef struct {
    shell_row_t rows[SHELL_OUTPUT_LINES];
    bool dirty;
} shell_screen_t;

void shell_handle_event(const input_event_t *event);

void shell_execute_command(const char *line);

void shell_writeln(const uint32_t *text);
void shell_writec(const uint32_t c);
layer_t *shell_get_layer();
shell_screen_t *shell_get_screen();
