/**
 * @file shell.h
 * @brief Kernel shell / terminal interface.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

kerr_t shell_init();
bool shell_is_running();
void shell_deinit();
