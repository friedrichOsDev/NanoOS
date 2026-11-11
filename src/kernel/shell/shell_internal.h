/**
 * @file shell_internal.h
 * @brief Private shell interfaces shared by shell modules.
 * @author friedrichOsDev
 */

#pragma once

#include <drivers/gfx/compositor.h>
#include <drivers/hid/input.h>
#include <kernel/sync.h>
#include <pch.h>

#define SHELL_PROMPT "friedrichOsDev@nanoos> "
#define SHELL_INPUT_MAX 256U
#define SHELL_HISTORY_COUNT 16U
#define SHELL_OUTPUT_LINES 64U
#define SHELL_OUTPUT_WIDTH 640U
#define SHELL_OUTPUT_COLUMNS 160U

typedef struct {
    char text[SHELL_OUTPUT_WIDTH];
} shell_row_t;

typedef struct {
    shell_row_t rows[SHELL_OUTPUT_LINES];
    size_t line_count;
    size_t visible_rows;
    size_t columns;
    size_t column;
    uint32_t input[SHELL_INPUT_MAX + 1U];
    size_t input_length;
    size_t input_cursor;
    bool dirty;
} shell_screen_t;

void shell_execute_command(const char *line);

void shell_write(const char *text);
void shell_writeln(const char *text);
void shell_writec(uint32_t codepoint);
void shell_clear(void);

irq_flags_t shell_screen_lock(void);
void shell_screen_unlock(irq_flags_t flags);
void shell_renderer_finished(void);

layer_t *shell_get_layer(void);
shell_screen_t *shell_get_screen(void);
