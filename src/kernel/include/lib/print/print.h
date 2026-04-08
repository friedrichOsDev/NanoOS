/**
 * @file print.h
 * @author friedrichOsDev 
 */

 #pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

int printf(const uint32_t* format, ...);
int usnprintf(uint32_t* dest, size_t size, const uint32_t* format, ...);
int uvsnprintf(uint32_t* dest, size_t size, const uint32_t* format, va_list args);
int snprintf(char* dest, size_t size, const char* format, ...);
int vsnprintf(char* dest, size_t size, const char* format, va_list args);