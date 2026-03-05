/**
 * @file console.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <fb.h>
#include <font.h>

void console_init(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void console_putc(uint32_t unicode);
void console_puts(const uint32_t* str);
void console_set_color(font_color_t color);
void console_set_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
font_color_t console_get_color(void);
void console_clear(void);