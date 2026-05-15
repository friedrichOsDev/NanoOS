/**
 * @file convert.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

int uint_to_str(uint64_t value, uint32_t* buffer, int base);
int uint_to_str_legacy(uint64_t value, char* buffer, int base);
char* ustr_to_str(const uint32_t* ustr, char* buffer, size_t buffer_size);
uint64_t str_to_u64(const uint32_t* str);
uint64_t str_to_u64_legacy(const char* str);
uint8_t bcd_to_dezimal(uint8_t bcd);
