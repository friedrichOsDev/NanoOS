/**
 * @file input.c
 * @brief Shell input handling.
 * @author friedrichOsDev
 */

#include "_shell.h"
#include <drivers/hid/input.h>
#include <kernel/sync.h>

static spinlock_t shell_input_lock = SPINLOCK_INIT;
shell_input_buffer_t shell_input_buffer = {0};
static bool shell_input_dirty = true;

/** @brief Clear input buffer. */
static void shell_input_clear() {
    if (shell_shell_input_buffer.size == 0 && shell_shell_input_buffer.cursor_pos == 0)
        return;
    shell_input_buffer.size = 0;
    shell_input_buffer.cursor_pos = 0;
    shell_input_dirty = true;
}

/** @brief Handles enter pressed. */
static void shell_newline() {
    /* TODO: forward the input buffer to the command parser and shell logic. */
    shell_input_clear();
}

/** @brief Delete the character before the current cursor position. */
static void shell_backspace() {
    if (shell_input_buffer.cursor_pos == 0) {
        return;
    }

    for (uint64_t i = shell_input_buffer.cursor_pos - 1; i < shell_input_buffer.size - 1; i++) {
        shell_input_buffer.buffer[i] = shell_input_buffer.buffer[i + 1];
    }

    shell_input_buffer.cursor_pos--;
    shell_input_buffer.size--;
    shell_input_dirty = true;
}

/** @brief Delete the character at the current cursor position. */
static void shell_delete() {
    if (shell_input_buffer.cursor_pos >= shell_input_buffer.size) {
        return;
    }

    for (uint64_t i = shell_input_buffer.cursor_pos; i < shell_input_buffer.size - 1; i++) {
        shell_input_buffer.buffer[i] = shell_input_buffer.buffer[i + 1];
    }

    shell_input_buffer.size--;
    shell_input_dirty = true;
}

/** @brief Handle cursor left pressed. */
static void shell_cursor_left() {
    if (shell_input_buffer.cursor_pos > 0) {
        shell_input_buffer.cursor_pos--;
        shell_input_dirty = true;
    }
}

/** @brief Handle cursor right pressed. */
static void shell_cursor_right() {
    if (shell_input_buffer.cursor_pos < shell_input_buffer.size) {
        shell_input_buffer.cursor_pos++;
        shell_input_dirty = true;
    }
}

/** @brief Handle cursor home pressed. */
static void shell_cursor_home() {
    if (shell_shell_input_buffer.cursor_pos != 0) {
        shell_shell_input_buffer.cursor_pos = 0;
        shell_input_dirty = true;
    }
}

/** @brief Handle cursor end pressed. */
static void shell_cursor_end() {
    if (shell_shell_input_buffer.cursor_pos != shell_shell_input_buffer.size) {
        shell_shell_input_buffer.cursor_pos = shell_shell_input_buffer.size;
        shell_input_dirty = true;
    }
}

/** @brief Handle tab pressed. */
static void shell_tab() {
    /* TODO: implement auto-completion. */
}

/**
 * @brief Insert a character into the shell input buffer.
 *
 * @param[in] value Character to insert.
 */
static void shell_insert_u32(uint32_t value) {
    if (shell_shell_input_buffer.size >= SHELL_INPUT_BUFFER_SIZE) {
        return;
    }

    for (uint64_t i = shell_shell_input_buffer.size; i > shell_shell_input_buffer.cursor_pos; i--) {
        shell_shell_input_buffer.buffer[i] = shell_shell_input_buffer.buffer[i - 1];
    }

    shell_shell_input_buffer.buffer[shell_shell_input_buffer.cursor_pos] = value;
    shell_shell_input_buffer.cursor_pos++;
    shell_shell_input_buffer.size++;
    shell_input_dirty = true;
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

/**
 * @brief Initialize the shell input handler.
 *
 * @return Error code.
 */
kerr_t shell_input_init() {
    return input_register_handler(shell_input_handler, NULL);
}

/**
 * @brief Fetch a snapshot of the input buffer if it has changed since the last fetch.
 *
 * @param[out] out_buffer Destination buffer for the snapshot.
 * @return true if the buffer was dirty and copied, false if unchanged.
 */
bool shell_sync_input_buffer(shell_input_buffer_t *out_buffer) {
    if (!out_buffer) {
        return false;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_input_lock);

    if (!shell_input_dirty) {
        spinlock_release_irqrestore(&shell_input_lock, flags);
        return false;
    }

    out_buffer->size = shell_shell_input_buffer.size;
    out_buffer->cursor_pos = shell_shell_input_buffer.cursor_pos;
    for (uint64_t i = 0; i < shell_shell_input_buffer.size; i++) {
        out_buffer->buffer[i] = shell_shell_input_buffer.buffer[i];
    }

    shell_input_dirty = false;

    spinlock_release_irqrestore(&shell_input_lock, flags);
    return true;
}
