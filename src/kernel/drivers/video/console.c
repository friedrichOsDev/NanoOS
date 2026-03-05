/**
 * @file console.c
 * @author friedrichOsDev
 */

#include <console.h>
#include <font.h>
#include <serial.h>
#include <kernel.h>

static uint32_t console_x = 0;
static uint32_t console_old_x = 0;
static uint32_t console_y = 0;
static uint32_t console_x_max = 0;
static uint32_t console_y_max = 0;
static uint32_t console_x_start = 0;
static uint32_t console_y_start = 0;
static font_color_t default_color;
static font_color_t console_color;

/**
 * @brief Initializes the console subsystem.
 * 
 * Sets up the framebuffer, default colors, and the console window boundaries.
 * @param x The starting X coordinate of the console window.
 * @param y The starting Y coordinate of the console window.
 * @param w The width of the console window.
 * @param h The height of the console window.
 */
void console_init(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    serial_printf("Console: start\n");
    fb_init();
    serial_printf("Console: set console attributes\n");
    default_color = (font_color_t){ .fg_color = white, .bg_color = black };
    console_set_color(default_color);
    console_set_window(x, y, w, h);
    serial_printf("Console: clear screen\n");
    console_clear();
    serial_printf("Console: done\n");

    init_state = INIT_CONSOLE;
}

/**
 * @brief Prints a single Unicode character to the console.
 * @param unicode The Unicode character to print.
 */
void console_putc(uint32_t unicode) {
    uint32_t screen_x = 0;
    uint32_t screen_y = 0;
    uint32_t font_width = font_get_width();
    uint32_t font_height = font_get_height();

    switch (unicode) {
        case 0x000A: {
            console_old_x = console_x;
            console_x = 0;
            console_y += font_height;
            if (console_y_start + console_y + font_height > console_y_max) {
                fb_scroll_rect(console_x_start, console_y_start, console_x_max - console_x_start, console_y_max - console_y_start, 1, console_color.bg_color);
                console_y -= font_height;
            }
            break;
        }
        case 0x0008: {
            if (console_x > 0) {
                console_x -= font_width;
            } else if (console_y > 0) {
                console_y -= font_height;
                console_x = ( (console_x_max - console_x_start) / font_width - 1) * font_width;
            }
            fb_draw_rect(console_x_start + console_x, console_y_start + console_y, font_width, font_height, console_color.bg_color);
            break;
        }
        case 0x0009: {
            uint32_t spaces = 4;
            for (uint32_t i = 0; i < spaces; i++) console_putc(0x0020);
            break;
        }
        default: {
            if (console_x_start + console_x + font_width > console_x_max) {
                console_old_x = console_x;
                console_x = 0;
                console_y += font_height;
            }

            if (console_y_start + console_y + font_height > console_y_max) {
                fb_scroll_rect(console_x_start, console_y_start, console_x_max - console_x_start, console_y_max - console_y_start, 1, console_color.bg_color);
                console_y -= font_height;
            }

            screen_x = console_x_start + console_x;
            screen_y = console_y_start + console_y;

            fb_draw_unicode(unicode, screen_x, screen_y, console_color.fg_color, console_color.bg_color);
            console_old_x = console_x;
            console_x += font_width;
            break;
        }
    }
}

/**
 * @brief Prints a null-terminated Unicode string to the console.
 * @param str The string to print.
 */
void console_puts(const uint32_t* str) {
    while (*str) {
        console_putc(*str++);
    }
}

/**
 * @brief Sets the current font colors for the console.
 * @param color A font_color_t struct containing foreground and background colors.
 */
void console_set_color(font_color_t color) {
    console_color = color;
}

/**
 * @brief Sets the boundaries of the console window.
 * 
 * Aligns the width and height to the font size and resets the cursor position.
 * @param x The starting X coordinate.
 * @param y The starting Y coordinate.
 * @param w The width of the window.
 * @param h The height of the window.
 */
void console_set_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    uint32_t font_width = font_get_width();
    uint32_t font_height = font_get_height();
    if(font_width == 0 || font_height == 0) {
        serial_printf("CONSOLE: Font not initialized, cannot set window.\n");
        return;
    }

    if (w < font_width) {
        w = font_width;
    }
    if (h < font_height) {
        h = font_height;
    }

    uint32_t aligned_w = (w / font_width) * font_width;
    uint32_t aligned_h = (h / font_height) * font_height;

    console_x_start = x;
    console_y_start = y;
    console_x_max = x + aligned_w;
    console_y_max = y + aligned_h;

    console_x = 0;
    console_old_x = 0;
    console_y = 0;
}

/**
 * @brief Returns the current font colors used by the console.
 * @return The current font_color_t.
 */
font_color_t console_get_color(void) {
    return console_color;
}

/**
 * @brief Clears the console window with the current background color.
 * 
 * Resets the cursor position to the top-left of the window.
 */
void console_clear(void) {
    fb_draw_rect(console_x_start, console_y_start, console_x_max - console_x_start, console_y_max - console_y_start, console_color.bg_color);
    console_x = 0;
    console_old_x = 0;
    console_y = 0;
}