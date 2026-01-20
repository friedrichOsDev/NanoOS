/*
 * @file console.c
 * @brief Console driver implementation
 * @author friedrichOsDev
 */

#include <console.h>
#include <fb.h>
#include <kernel.h>
#include <font.h>

/*
 * Current cursor x position
 * @note x value is in pixels
 */
static uint32_t console_x;

/*
 * Current cursor y position
 * @note y value is in pixels
 */
static uint32_t console_y;

/*
 * Current foreground color
 */
static uint32_t console_fg_color;

/*
 * Current background color
 */
static uint32_t console_bg_color;

/*
 * A function to initialize the console
 * @param void
 */
void console_init(void) {
    fb_init();
    console_clear();
    console_set_color(0xFFFFFF, 0x000000);
}

/*
 * Output a character to the console
 * @param c The character to output
 */
void console_putc(char c) {
    if (c == '\n') {
        console_x = 0;
        console_y += FONT_HEIGHT;
        if (console_y >= fb_get_height()) {
            fb_scroll(FONT_HEIGHT, 0x000000);
            console_y -= FONT_HEIGHT;
        }
    } else if (c == '\b') {
        if (console_x >= FONT_WIDTH) {
            console_x -= FONT_WIDTH;
        } else if (console_y >= FONT_HEIGHT) {
            console_y -= FONT_HEIGHT;
            
            int found_x = 0;
            int found = 0;
            int start_x = fb_get_width();

            for (int x = start_x; x >= 0; x -= FONT_WIDTH) {
                for (int py = 0; py < FONT_HEIGHT; py++) {
                    for (int px = 0; px < FONT_WIDTH; px++) {
                        if (fb_get_pixel(x + px, console_y + py) != console_bg_color) {
                            found_x = x + FONT_WIDTH;
                            found = 1;
                            goto scan_end;
                        }
                    }
                }
            }
            scan_end:
            console_x = found ? (found_x - FONT_WIDTH) : 0;
        }
        fb_draw_char(console_x, console_y, ' ', console_fg_color, console_bg_color);
    } else if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            console_putc(' ');
        }
    } else {
        fb_draw_char(console_x, console_y, c, console_fg_color, console_bg_color);
        console_x += FONT_WIDTH;

        if (console_x >= fb_get_width()) {
            console_x = 0;
            console_y += FONT_HEIGHT;
        }

        if (console_y >= fb_get_height()) {
            fb_scroll(FONT_HEIGHT, 0x000000);
            console_y -= FONT_HEIGHT;
        }
    }
}

/*
 * Output a string to the console
 * @param str The string to output
 */
void console_puts(const char* str) {
    while (*str) {
        console_putc(*str++);
    }
}

/*
 * Set the console colors
 * @param fg_color The foreground color
 * @param bg_color The background color
 */
void console_set_color(uint32_t fg_color, uint32_t bg_color) {
    console_fg_color = fg_color;
    console_bg_color = bg_color;
}

/*
 * Set the foreground color
 * @param fg_color The foreground color
 */
void console_set_fg_color(uint32_t fg_color) {
    console_fg_color = fg_color;
}

/*
 * Set the background color
 * @param bg_color The background color
 */
void console_set_bg_color(uint32_t bg_color) {
    console_bg_color = bg_color;
}

/*
 * Get the foreground color
 * @param void
 * @return The foreground color
 */
uint32_t console_get_fg_color(void) {
    return console_fg_color;
}

/*
 * Get the background color
 * @param void
 * @return The background color
 */
uint32_t console_get_bg_color(void) {
    return console_bg_color;
}

/*
 * Clear the console
 * @param void
 */
void console_clear(void) {
    fb_clear(0x000000);
    console_x = 0;
    console_y = 0;
}