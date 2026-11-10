/**
 * @file shell.h
 * @brief Public kernel shell / terminal interface.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

kerr_t shell_init(void);
bool shell_is_running(void);
void shell_deinit(void);
