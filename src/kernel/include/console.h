#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stddef.h>
#include <fb.h>
#include <kernel.h>
#include <font.h>

void console_init();
void console_putc(char c);
void console_puts(const char* str);
void console_set_color(uint32_t fg_color, uint32_t bg_color);
void console_set_fg_color(uint32_t fg_color);
uint32_t console_get_fg_color();
void console_set_bg_color(uint32_t bg_color);
uint32_t console_get_bg_color();
void console_clear();

#endif // CONSOLE_H
