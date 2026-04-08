/**
 * @file console.c
 * @author friedrichOsDev
 */

#include <console.h>
#include <font.h>
#include <serial.h>
#include <kernel.h>
#include <memory.h>
#include <timer.h>
#include <keyboard.h>

static uint32_t console_x = 0;
static uint32_t console_y = 0;
static uint32_t console_x_max = 0;
static uint32_t console_y_max = 0;
static uint32_t console_x_start = 0;
static uint32_t console_y_start = 0;
static font_color_t default_color;
static font_color_t console_color;
static console_buffer_t console_buffer;
static bool update_flag = false;
static bool cursor_visible = true;
static uint32_t cursor_unicode = U'_';
static uint32_t cursor_old_x = 0;
static uint32_t cursor_old_y = 0;

static void console_update_event(void) {
    update_flag = true;
}

static void console_cursor_blink_event(void) {
    cursor_visible = !cursor_visible;
}

/**
 * @brief Checks if an update is pending and performs a buffer draw if necessary.
 * Should be called in the main kernel loop.
 */
void console_update(void) {
    if (cursor_old_x != console_x || cursor_old_y != console_y) {
        uint32_t buffer_size = console_buffer.width * console_buffer.height;
        uint32_t offset = CONSOLE_XY_TO_IDX(cursor_old_x, cursor_old_y, console_buffer.width);
        uint32_t idx = (console_buffer.head + offset) % buffer_size;
        console_buffer.buffer[idx] |= CONSOLE_BUFFER_DIRTY_BIT;

        cursor_old_x = console_x;
        cursor_old_y = console_y;
    }

    if (update_flag) {
        console_draw_buffers();
        update_flag = false;
    }

    // Draw cursor
    if (cursor_visible) {
        uint32_t x = console_x * font_get_width() + console_x_start;
        uint32_t y = console_y * font_get_height() + console_y_start;
        fb_draw_unicode(cursor_unicode, x, y, console_color.fg_color, console_color.bg_color);
    } else {
        uint32_t x = console_x * font_get_width() + console_x_start;
        uint32_t y = console_y * font_get_height() + console_y_start;
        uint32_t buffer_size = console_buffer.width * console_buffer.height;
        uint32_t offset = CONSOLE_XY_TO_IDX(console_x, console_y, console_buffer.width);
        uint32_t idx = (console_buffer.head + offset) % buffer_size;
        uint32_t unicode = console_buffer.buffer[idx] & ~CONSOLE_BUFFER_DIRTY_BIT;
        fb_draw_unicode(unicode, x, y, console_color.fg_color, console_color.bg_color);
    }
}

bool console_move_cursor_left(bool release) {
    if (release) return true;

    if (console_x > 0) {
        console_x--;
    } else if (console_y > 0) {
        console_y--;
        console_x = console_buffer.width - 1;
    }
    return true;
}

bool console_move_cursor_right(bool release) {
    if (release) return true;

    if (console_x < console_buffer.width - 1) {
        console_x++;
    } else if (console_y < console_buffer.height - 1) {
        console_x = 0;
        console_y++;
    }
    
    return true;
}

/**
 * @brief Initializes the console subsystem.
 * 
 * Sets up the framebuffer, default colors, and the console window boundaries.
 * @param x The starting X coordinate of the console window.
 * @param y The starting Y coordinate of the console window.
 * @param w The width of the console window.
 * @param h The height of the console window.
 * @param color The default font color for the console.
 */
void console_init(uint32_t x, uint32_t y, uint32_t w, uint32_t h, font_color_t color) {
    fb_init();
    default_color = color;
    console_set_color(color);
    console_set_window(x, y, w, h);
    console_clear();

    event_t update_event = {
        .event_id = 0,
        .handler = console_update_event,
        .interval = 1,
        .target_tick = timer_get_ticks() + 1,
        .repeat = true,
        .active = true
    };
    event_t cursor_blink_event = {
        .event_id = 0,
        .handler = console_cursor_blink_event,
        .interval = TIMER_FREQUENCY / 2,
        .target_tick = timer_get_ticks() + TIMER_FREQUENCY / 2,
        .repeat = true,
        .active = true
    };

    uint32_t update_event_id = timer_add_event(update_event);
    uint32_t cursor_blink_event_id = timer_add_event(cursor_blink_event);
    (void)update_event_id;
    (void)cursor_blink_event_id;

    keyboard_map_function_to_vk(VK_LEFT, console_move_cursor_left);
    keyboard_map_function_to_vk(VK_RIGHT, console_move_cursor_right);

    init_state = INIT_CONSOLE;
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
    // align to font size
    uint32_t font_width = font_get_width();
    uint32_t font_height = font_get_height();

    console_x_start = x;
    console_y_start = y;
    console_x_max = x + (w / font_width) * font_width;
    console_y_max = y + (h / font_height) * font_height;

    console_x = 0;
    console_y = 0;

    // init and alloc console buffer
    console_buffer.width = w / font_width;
    console_buffer.height = h / font_height;
    console_buffer.buffer = (uint32_t*)kzalloc(console_buffer.width * console_buffer.height * sizeof(uint32_t));
    console_buffer.head = 0;
    console_buffer.tail = 0;
}

/**
 * @brief Prints a single Unicode character to the console.
 * @param unicode The Unicode character to print.
 */
void console_putc(uint32_t unicode) {
    uint32_t buffer_size = console_buffer.width * console_buffer.height;
    switch (unicode) {
        case U'\n': {
            console_x = 0;
            console_y++;
            if (console_y >= console_buffer.height) {
                console_y = console_buffer.height - 1;
                console_scroll();
            }
            break;
        }
        case U'\b': {
            if (console_x > 0) {
                console_x--;
            } else if (console_y > 0) {
                console_y--;
                console_x = console_buffer.width - 1;
            }
            uint32_t offset = CONSOLE_XY_TO_IDX(console_x, console_y, console_buffer.width);
            uint32_t idx = (console_buffer.head + offset) % buffer_size;
            console_buffer.buffer[idx] = U' ' | CONSOLE_BUFFER_DIRTY_BIT;
            break;
        }
        case U'\t': {
            for (uint32_t i = 0; i < CONSOLE_TAB_SPACES; i++) console_putc(U' ');
            break;
        }
        default: {
            uint32_t offset = CONSOLE_XY_TO_IDX(console_x, console_y, console_buffer.width);
            uint32_t idx = (console_buffer.head + offset) % buffer_size;
            console_buffer.buffer[idx] = unicode | CONSOLE_BUFFER_DIRTY_BIT;
            console_x++;
            if (console_x >= console_buffer.width) {
                console_x = 0;
                console_y++;
            }
            if (console_y >= console_buffer.height) {
                console_y = console_buffer.height - 1;
                console_scroll();
            }
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
    for (uint32_t i = 0; i < console_buffer.width * console_buffer.height; i++) {
        console_buffer.buffer[i] = U' ' | CONSOLE_BUFFER_DIRTY_BIT;
    }
    console_buffer.head = 0;
    console_buffer.tail = 0;
    console_x = 0;
    console_y = 0;
}

void console_scroll(void) {
    uint32_t line_to_clear = console_buffer.head;
    console_buffer.head = (console_buffer.head + console_buffer.width) % (console_buffer.width * console_buffer.height);
    for (uint32_t i = 0; i < console_buffer.width; i++) {
        console_buffer.buffer[line_to_clear + i] = U' ' | CONSOLE_BUFFER_DIRTY_BIT;
    }

    for (uint32_t i = 0; i < console_buffer.width * console_buffer.height; i++) {
        console_buffer.buffer[i] |= CONSOLE_BUFFER_DIRTY_BIT;
    }
}

void console_draw_buffers(void) {
    if (console_buffer.buffer == NULL) {
        serial_printf("Console: Error: Console buffer not initialized\n");
        return;
    }

    uint32_t idx = console_buffer.head;
    for (uint32_t i = 0; i < console_buffer.width * console_buffer.height; i++) {
        uint32_t char_with_dirty = console_buffer.buffer[idx];
        if (char_with_dirty & CONSOLE_BUFFER_DIRTY_BIT) {
            uint32_t unicode = char_with_dirty & ~CONSOLE_BUFFER_DIRTY_BIT;
            uint32_t x = (i % console_buffer.width) * font_get_width() + console_x_start;
            uint32_t y = (i / console_buffer.width) * font_get_height() + console_y_start;
            fb_draw_unicode(unicode, x, y, console_color.fg_color, console_color.bg_color);
            console_buffer.buffer[idx] &= ~CONSOLE_BUFFER_DIRTY_BIT; // clear dirty bit
        }
        idx = (idx + 1) % (console_buffer.width * console_buffer.height);
    }
}

console_buffer_t* console_get_buffer(void) {
    return &console_buffer;
}