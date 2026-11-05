/**
 * @file conv.h
 * @brief Convertion functions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

int uint_to_str(uint64_t value, uint32_t *buffer, unsigned int base);
int uint_to_wstr(uint64_t value, char *buffer, unsigned int base);
int double_to_str(double value, char *buf, int prec);
int double_to_wstr(double value, uint32_t *buf, int prec);
