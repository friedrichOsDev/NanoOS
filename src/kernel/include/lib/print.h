/**
 * @file print.h
 * @brief Printf implementation code (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

int usnprintf(uint32_t* dest, size_t size, const uint32_t* format, ...);
int uvsnprintf(uint32_t* dest, size_t size, const uint32_t* format, va_list args);
int snprintf(char* dest, size_t size, const char* format, ...);
int vsnprintf(char* dest, size_t size, const char* format, va_list args);