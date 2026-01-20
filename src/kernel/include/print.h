/*
 * @file print.h
 * @brief Header for printf implementation
 * @author friedrichOsDev
 */

#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

void printf(const char* format, ...);
void vprintf(const char* format, va_list args);

#endif // PRINT_H
