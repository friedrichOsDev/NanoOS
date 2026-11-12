/**
 * @file input.c
 * @brief Shell input handling.
 * @author friedrichOsDev
 */

#include "_shell.h"
#include <drivers/hid/input.h>
#include <kernel/sync.h>

static spinlock_t shell_input_lock = SPINLOCK_INIT;

static void shell_newline() {
    log_debug("Newline pressed");
}

static void shell_backspace() {
    log_debug("Backspace pressed");
}

static void shell_delete() {
    log_debug("Delete pressed");
}

static void shell_input_clear() {
    log_debug("Input clear pressed");
}

static void shell_cursor_left() {
    log_debug("Cursor left pressed");
}

static void shell_cursor_right() {
    log_debug("Cursor right pressed");
}

static void shell_cursor_home() {
    log_debug("Cursor home pressed");
}

static void shell_cursor_end() {
    log_debug("Cursor end pressed");
}

static void shell_tab() {
    log_debug("Tab pressed");
}

static void shell_insert_u32(uint32_t value) {
    log_var(value);
}

/**
 * @brief Handle shell input events.
 *
 * @param[in] event Translated input event.
 * @param[in] context Unused callback context.
 */
static void shell_input_handler(const input_event_t *event, void *context) {
    (void)context;

    if (!event || event->hid.type != HID_EVENT_KEY_PRESS ||
        (event->key_action != INPUT_KEY_ACTION_PRESSED && event->key_action != INPUT_KEY_ACTION_REPEATED))
        return;

    if (!shell_initialized)
        return;

    hid_keycode_t key = event->hid.key.keycode;
    if ((size_t)key >= INPUT_KEY_COUNT)
        return;

    keyboard_state_t keyboard;
    if (k_failed(keyboard_get_state(&keyboard)) ||
        keyboard.control || keyboard.alt)
        return;

    bool numpad_nav_active = !keyboard.num_lock;

    bool is_no_repeat_key =
        key == KEY_RETURN || key == KEY_NUMLOCK_RETURN ||
        key == KEY_NUMLOCK_CLEAR ||
        key == KEY_HOME || key == KEY_END ||
        (numpad_nav_active && (key == KEY_NUMLOCK_HOME || key == KEY_NUMLOCK_END));

    if (is_no_repeat_key && event->key_action != INPUT_KEY_ACTION_PRESSED)
        return;

    bool is_control_key =
        key == KEY_RETURN || key == KEY_NUMLOCK_RETURN ||
        key == KEY_NUMLOCK_CLEAR ||
        key == KEY_BACK || key == KEY_DELETE ||
        key == KEY_LEFT || key == KEY_RIGHT ||
        key == KEY_UP || key == KEY_DOWN ||
        key == KEY_HOME || key == KEY_END ||
        (numpad_nav_active && (key == KEY_NUMLOCK_DELETE ||
                               key == KEY_NUMLOCK_LEFT || key == KEY_NUMLOCK_RIGHT ||
                               key == KEY_NUMLOCK_UP || key == KEY_NUMLOCK_DOWN ||
                               key == KEY_NUMLOCK_HOME || key == KEY_NUMLOCK_END));

    if (event->unicode_count == 0U && !is_control_key)
        return;

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_input_lock);

    if (key == KEY_RETURN || key == KEY_NUMLOCK_RETURN) {
        shell_newline();
    } else if (key == KEY_NUMLOCK_CLEAR) {
        shell_input_clear();
    } else if (key == KEY_BACK) {
        shell_backspace();
    } else if (key == KEY_DELETE || (numpad_nav_active && key == KEY_NUMLOCK_DELETE)) {
        shell_delete();
    } else if (key == KEY_LEFT || (numpad_nav_active && key == KEY_NUMLOCK_LEFT)) {
        shell_cursor_left();
    } else if (key == KEY_RIGHT || (numpad_nav_active && key == KEY_NUMLOCK_RIGHT)) {
        shell_cursor_right();
    } else if (key == KEY_UP || (numpad_nav_active && key == KEY_NUMLOCK_UP)) {
        // TODO: shell history navigation
    } else if (key == KEY_DOWN || (numpad_nav_active && key == KEY_NUMLOCK_DOWN)) {
        // TODO: shell history navigation
    } else if (key == KEY_HOME || (numpad_nav_active && key == KEY_NUMLOCK_HOME)) {
        shell_cursor_home();
    } else if (key == KEY_END || (numpad_nav_active && key == KEY_NUMLOCK_END)) {
        shell_cursor_end();
    } else {
        for (uint8_t i = 0; i < event->unicode_count; i++) {
            uint32_t codepoint = (i == 0) ? event->unicode : event->unicode_secondary;
            if (codepoint == (uint32_t)'\t') {
                shell_tab();
            } else {
                shell_insert_u32(codepoint);
            }
        }
    }

    spinlock_release_irqrestore(&shell_input_lock, flags);
}

kerr_t shell_input_init() {
    return input_register_handler(shell_input_handler, NULL);
}
