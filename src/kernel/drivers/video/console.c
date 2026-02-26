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
 * @brief Prints a single character to the console.
 * 
 * Handles special characters like newline, backspace, and tab.
 * Automatically handles word wrapping and scrolling when the window
 * boundaries are reached.
 * @param c The character to print.
 */
void console_putc(char c) {
    uint32_t screen_x = 0;
    uint32_t screen_y = 0;
    switch (c) {
        case '\n': {
            console_old_x = console_x;
            console_x = 0;
            console_y += FONT_HEIGHT;
            if (console_y >= console_y_max - console_y_start) {
                fb_scroll_rect(console_x_start, console_y_start, console_x_max - console_x_start, console_y_max - console_y_start, FONT_HEIGHT, console_color.bg_color);
                console_y -= FONT_HEIGHT;
            }
            break;
        }
        case '\b': {
            if (console_x > 0) {
                console_x -= FONT_WIDTH;
            } else if (console_y > 0) {
                console_x = console_old_x;
                console_y -= FONT_HEIGHT;
            }
            fb_draw_rect(console_x_start + console_x, console_y_start + console_y, FONT_WIDTH, FONT_HEIGHT, console_color.bg_color);
            break;
        }
        case '\t': {
            uint32_t spaces = 4;
            for (uint32_t i = 0; i < spaces; i++) console_putc(' ');
            break;
        }
        default: {
            if (console_x_start + console_x + FONT_WIDTH > console_x_max) {
                console_x = 0;
                console_y += FONT_HEIGHT;
            }
            
            if (console_y_start + console_y + FONT_HEIGHT > console_y_max) {
                fb_scroll_rect(console_x_start, console_y_start, console_x_max - console_x_start, console_y_max - console_y_start, FONT_HEIGHT, console_color.bg_color);
                console_y -= FONT_HEIGHT;
            }

            screen_x = console_x_start + console_x;
            screen_y = console_y_start + console_y;

            fb_draw_char(screen_x, screen_y, c, console_color.fg_color, console_color.bg_color);
            console_old_x = console_x;
            console_x += FONT_WIDTH;
            break;
        }
    }
}

/**
 * @brief Prints a null-terminated string to the console.
 * 
 * Iterates through the string and calls console_putc for each character.
 * @param str The string to print.
 */
void console_puts(const char* str) {
    while (*str) {
        console_putc(*str++);
    }
}

/**
 * @brief Sets the current font colors for the console.
 * 
 * Subsequent calls to console_putc will use these colors.
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
    if (w < FONT_WIDTH) {
        w = FONT_WIDTH;
    }
    if (h < FONT_HEIGHT) {
        h = FONT_HEIGHT;
    }

    uint32_t aligned_w = (w / FONT_WIDTH) * FONT_WIDTH;
    uint32_t aligned_h = (h / FONT_HEIGHT) * FONT_HEIGHT;

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