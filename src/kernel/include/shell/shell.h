/**
 * @file shell.h
 * @brief Public kernel shell / terminal interface.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

kerr_t shell_init();
void shell_deinit();
bool shell_is_running();
