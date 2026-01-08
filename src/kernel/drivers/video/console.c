#include <console.h>

static uint32_t console_x;
static uint32_t console_y;
static uint32_t console_fg_color;
static uint32_t console_bg_color;

void console_init() {
    fb_init();
    console_clear();
    console_set_color(0xFFFFFF, 0x000000);
}

void console_putc(char c) {
    if (c == '\n') {
        console_x = 0;
        console_y += FONT_HEIGHT;
        if (console_y >= fb_get_height()) {
            fb_scroll(FONT_HEIGHT, 0x000000);
            console_y -= FONT_HEIGHT;
        }
        return;
    }

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

void console_puts(const char* str) {
    while (*str) {
        console_putc(*str++);
    }
    
}

void console_set_color(uint32_t fg_color, uint32_t bg_color) {
    console_fg_color = fg_color;
    console_bg_color = bg_color;
}

void console_set_fg_color(uint32_t fg_color) {
    console_fg_color = fg_color;
}

uint32_t console_get_fg_color() {
    return console_fg_color;
}

void console_set_bg_color(uint32_t bg_color) {
    console_bg_color = bg_color;
}

uint32_t console_get_bg_color() {
    return console_bg_color;
}

void console_clear() {
    fb_clear(0x000000);
    console_x = 0;
    console_y = 0;
}