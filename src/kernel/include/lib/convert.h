/**
 * @file convert.h
 * @brief Conversion Functions
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

int uint_to_str(uint64_t value, uint32_t *buffer, int base);
int uint_to_str_legacy(uint64_t value, char *buffer, int base);
int double_to_str(double value, char *buf, int prec);
int double_to_wstr(double value, uint32_t *buf, int prec);