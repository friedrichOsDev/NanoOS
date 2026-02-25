/**
 * @file print.h
 * @author friedrichOsDev 
 */

 #pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

int printf(const char* format, ...);
int snprintf(char* dest, size_t size, const char* format, ...);
int vsnprintf(char* dest, size_t size, const char* format, va_list args);