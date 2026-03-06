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

#define CONSOLE_BUFFER_DIRTY_BIT 0x80000000
#define CONSOLE_TAB_SPACES 4

#define CONSOLE_XY_TO_IDX(x, y, width) ((y) * (width) + (x))

/**
 * @brief Represents a circular buffer for storing console characters.
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* buffer;
    uint32_t head;
    uint32_t tail;
} console_buffer_t;

void console_update(void);
void console_init(uint32_t x, uint32_t y, uint32_t w, uint32_t h, font_color_t color);
void console_set_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void console_putc(uint32_t unicode);
void console_puts(const uint32_t* str);
void console_set_color(font_color_t color);
font_color_t console_get_color(void);
void console_clear(void);
void console_scroll(void);
void console_draw_buffers(void);
console_buffer_t* console_get_buffer(void);