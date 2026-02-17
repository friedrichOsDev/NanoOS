/*
 * @file panic.h
 * @brief Header file for kernel panic handling
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

void kernel_panic(const char* error_msg, uint32_t error_code);