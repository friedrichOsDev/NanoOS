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
uint8_t bcd_to_dezimal(uint8_t bcd);
char* ustr_to_str(const uint32_t* ustr, char* buffer, size_t buffer_size);
