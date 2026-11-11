/**
 * @file shell.h
 * @brief Public kernel shell / terminal interface.
 * @author friedrichOsDev
 */

#pragma once

#include <drivers/hid/input.h>
#include <pch.h>

kerr_t shell_init(void);
void shell_deinit(void);
bool shell_is_running(void);
void shell_handle_event(const input_event_t *event);
