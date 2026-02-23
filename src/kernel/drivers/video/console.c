/*
 * @file console.c
 * @brief text console interface
 * @author friedrichOsDev
 */

#include <console.h>
#include <font.h>
#include <serial.h>

static uint32_t console_x = 0;
static uint32_t console_old_x = 0;
static uint32_t console_y = 0;
static uint32_t console_x_max = 0;
static uint32_t console_y_max = 0;
static uint32_t console_x_start = 0;
static uint32_t console_y_start = 0;
static font_color_t default_color;
static font_color_t console_color;

/*
 * Initialize the console
 * @param x The x coordinate of the top-left corner of the console
 * @param y The y coordinate of the top-left corner of the console
 * @param w The width of the console in pixels
 * @param h The height of the console in pixels
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
}

/*
 * Print a character to the console, handling special characters like newlines and backspaces
 * @param c The character to print
 */
void console_putc(char c) {
    uint32_t screen_x = 0;
    uint32_t screen_y = 0;
    switch (c) {
        case '\n':
            console_old_x = console_x - FONT_WIDTH;
            console_x = 0;
            console_y += FONT_HEIGHT;
            if (console_y >= console_y_max - console_y_start) {
                fb_scroll_rect(console_x_start, console_y_start, console_x_max - console_x_start, console_y_max - console_y_start, FONT_HEIGHT, console_color.bg_color);
                console_y -= FONT_HEIGHT;
            }
            break;
        case '\b':
            if (console_x > 0) {
                console_x -= FONT_WIDTH;
            } else if (console_y > 0) {
                console_x = console_old_x;
                console_y -= FONT_HEIGHT;
            }
            fb_draw_char(console_x_start + console_x, console_y_start + console_y, ' ', console_color.fg_color, console_color.bg_color);
            break;
        case '\t':
            for (int i = 0; i < 4; i++) {
                console_putc(' ');
            }
            break;
        default:
            screen_x = console_x_start + console_x;
            screen_y = console_y_start + console_y;
            if (screen_x + FONT_WIDTH > console_x_max) {
                console_putc('\n');
                screen_x = console_x_start + console_x;
                screen_y = console_y_start + console_y;
            }
            fb_draw_char(screen_x, screen_y, c, console_color.fg_color, console_color.bg_color);
            console_old_x = console_x;
            console_x += FONT_WIDTH;
            break;
    }
}

/*
 * Print a string to the console
 * @param str The null-terminated string to print
 */
void console_puts(const char* str) {
    while (*str) {
        console_putc(*str++);
    }
}


/*
 * Set the console color
 * @param color The new foreground and background colors
 */
void console_set_color(font_color_t color) {
    console_color = color;
}

/*
 * Set the console position and size
 * @param x The x coordinate of the top-left corner of the console
 * @param y The y coordinate of the top-left corner of the console
 * @param w The width of the console in pixels
 * @param h The height of the console in pixels
 * @note The console will automatically wrap text and scroll when it reaches the bottom of the defined area
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
}

/*
 * Get the current console color
 * @param void
 * @return The current foreground and background colors
 */
font_color_t console_get_color(void) {
    return console_color;
}

/*
 * Clear the console and reset cursor position
 * @param void
 */
void console_clear(void) {
    fb_draw_rect(console_x_start, console_y_start, console_x_max - console_x_start, console_y_max - console_y_start, console_color.bg_color);
    console_x = 0;
    console_old_x = 0;
    console_y = 0;
}