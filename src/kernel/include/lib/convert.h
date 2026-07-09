/**
 * @file convert.h
 * @brief convertion functions (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

int uint_to_str(uint64_t value, uint32_t* buffer, int base);
int uint_to_str_legacy(uint64_t value, char* buffer, int base);