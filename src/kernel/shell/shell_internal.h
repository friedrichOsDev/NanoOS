/**
 * @file shell_internal.h
 * @brief Private shell interfaces shared by terminal modules.
 * @author friedrichOsDev
 */

#pragma once

#include <drivers/hid/input.h>
#include <drivers/gfx/compositor.h>
#include <pch.h>

#define SHELL_INPUT_MAX 256U
#define SHELL_HISTORY_COUNT 8U
#define SHELL_OUTPUT_LINES 32U
#define SHELL_OUTPUT_WIDTH 160U

void shell_handle_event(const input_event_t *event);
void shell_render_frame(void);
void shell_execute_command(const char *line);
void shell_write(const char *text);
void shell_writeln(const char *text);
void shell_write_u64(uint64_t value);
void shell_clear(void);
void shell_request_shutdown(void);
