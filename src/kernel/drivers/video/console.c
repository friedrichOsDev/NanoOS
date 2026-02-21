/*
 * @file console.c
 * @brief text console interface
 * @author friedrichOsDev
 */

#include <console.h>
#include <font.h>

static uint32_t console_x;
static uint32_t console_old_x;
static uint32_t console_y;
static font_color_t default_color;
static font_color_t console_color;

/*
 * Initialize the console
 * @param void
 */
void console_init(void) {
    fb_init();
    default_color = (font_color_t){ .fg_color = white, .bg_color = black };
    console_set_color(default_color);
    console_clear();
}

/*
 * Print a character to the console, handling special characters like newlines and backspaces
 * @param c The character to print
 */
void console_putc(char c) {
    switch (c) {
        case '\n':
            console_old_x = console_x;
            console_x = 0;
            console_y += FONT_HEIGHT;
            if (console_y >= fb_get_height()) {
                fb_scroll(FONT_HEIGHT, console_color.bg_color);
                console_y -= FONT_HEIGHT;
            }
            break;

        case '\b':
            if (console_x >= FONT_WIDTH) {
                console_x -= FONT_WIDTH;
            } else {
                if (console_y >= FONT_HEIGHT) {
                    console_y -= FONT_HEIGHT;
                    console_x = console_old_x;
                }
            }
            fb_draw_char(console_x, console_y, ' ', console_color.fg_color, console_color.bg_color);
            break;
        
        case '\t':
            for (int i = 0; i < 4; i++) {
                console_putc(' ');
            }
            break;
        
        default:
            fb_draw_char(console_x, console_y, c, console_color.fg_color, console_color.bg_color);
            console_x += FONT_WIDTH;
            if (console_x >= fb_get_width()) {
                console_old_x = console_x;
                console_x = 0;
                console_y += FONT_HEIGHT;
                if (console_y >= fb_get_height()) {
                    fb_scroll(FONT_HEIGHT, console_color.bg_color);
                    console_y -= FONT_HEIGHT;
                }
            }
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
    fb_clear(console_color.bg_color);
    console_x = 0;
    console_old_x = 0;
    console_y = 0;
}