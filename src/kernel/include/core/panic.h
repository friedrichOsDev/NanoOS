/**
 * @file panic.h
 * @brief Kernel panic
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

void panic(const char *message, uint64_t error_code);