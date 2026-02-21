/*
 * @file console.h
 * @brief Header file for text console interface
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <fb.h>

// one data type for foreground and background color
typedef struct {
    color_t fg_color;
    color_t bg_color;
} font_color_t;

void console_init(void);
void console_putc(char c);
void console_puts(const char* str);
void console_set_color(font_color_t color);
font_color_t console_get_color(void);
void console_clear(void);