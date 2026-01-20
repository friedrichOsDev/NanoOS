/*
 * @file console.h
 * @brief Header file for console driver implementation
 * @author friedrichOsDev
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stddef.h>

void console_init(void);
void console_putc(char c);
void console_puts(const char* str);
void console_set_color(uint32_t fg_color, uint32_t bg_color);
void console_set_fg_color(uint32_t fg_color);
void console_set_bg_color(uint32_t bg_color);
uint32_t console_get_fg_color(void);
uint32_t console_get_bg_color(void);
void console_clear(void);

#endif // CONSOLE_H
