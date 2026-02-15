/*
 * @file convert.h
 * @brief Header file for type conversion functions
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

const char* hex_to_str(uint32_t value);
const char* int_to_str(int32_t value);
int32_t str_to_int(const char* str);
uint32_t str_to_hex(const char* str);
