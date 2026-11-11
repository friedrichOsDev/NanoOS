/**
 * @file shell_input.c
 * @brief Keyboard editing, cursor navigation and command history.
 * @author friedrichOsDev
 */

#include "shell_input.h"
#include "shell_internal.h"
#include <lib/util/mem.h>

static uint32_t command_history[SHELL_HISTORY_COUNT][SHELL_INPUT_MAX];
static size_t command_history_lengths[SHELL_HISTORY_COUNT];
static size_t command_history_count;
static size_t history_cursor;

/**
 * @brief Stores the current command in the bounded history.
 * @param[in] screen Current terminal state, with its lock held.
 */
static void history_store(const shell_screen_t *screen) {
    if (screen->input_length == 0U) {
        history_cursor = command_history_count;
        return;
    }

    if (command_history_count > 0U) {
        size_t last = command_history_count - 1U;
        if (command_history_lengths[last] == screen->input_length &&
            memcmp(command_history[last], screen->input,
                   screen->input_length * sizeof(uint32_t)) == 0) {
            history_cursor = command_history_count;
            return;
        }
    }

    if (command_history_count == SHELL_HISTORY_COUNT) {
        memmove(command_history, command_history + 1,
                (SHELL_HISTORY_COUNT - 1U) * sizeof(command_history[0]));
        memmove(command_history_lengths, command_history_lengths + 1,
                (SHELL_HISTORY_COUNT - 1U) * sizeof(command_history_lengths[0]));
        command_history_count--;
    }

    size_t index = command_history_count;
    memcpy(command_history[index], screen->input,
           screen->input_length * sizeof(uint32_t));
    command_history_lengths[index] = screen->input_length;
    command_history_count++;
    history_cursor = command_history_count;
}

/**
 * @brief Replaces the current input with a history entry.
 * @param[in,out] screen Current terminal state, with its lock held.
 * @param[in] index History entry to load.
 */
static void history_load(shell_screen_t *screen, size_t index) {
    if (index >= command_history_count) {
        return;
    }

    size_t length = command_history_lengths[index];
    memcpy(screen->input, command_history[index], length * sizeof(uint32_t));
    screen->input_length = length;
    screen->input_cursor = length;
    screen->input[screen->input_length] = 0U;
    screen->dirty = true;
}

/**
 * @brief Inserts a Unicode codepoint at the current input cursor.
 * @param[in,out] screen Current terminal state, with its lock held.
 * @param[in] codepoint Codepoint to insert.
 */
static void input_insert(shell_screen_t *screen, uint32_t codepoint) {
    if (screen->input_length >= SHELL_INPUT_MAX) {
        return;
    }

    memmove(&screen->input[screen->input_cursor + 1U],
            &screen->input[screen->input_cursor],
            (screen->input_length - screen->input_cursor) * sizeof(uint32_t));
    screen->input[screen->input_cursor] = codepoint;
    screen->input_length++;
    screen->input_cursor++;
    screen->input[screen->input_length] = 0U;
    screen->dirty = true;
}

/**
 * @brief Deletes the codepoint immediately before the input cursor.
 * @param[in,out] screen Current terminal state, with its lock held.
 */
static void input_backspace(shell_screen_t *screen) {
    if (screen->input_cursor == 0U) {
        return;
    }

    memmove(&screen->input[screen->input_cursor - 1U],
            &screen->input[screen->input_cursor],
            (screen->input_length - screen->input_cursor) * sizeof(uint32_t));
    screen->input_cursor--;
    screen->input_length--;
    screen->input[screen->input_length] = 0U;
    screen->dirty = true;
}

/**
 * @brief Encodes the current codepoint command as UTF-8.
 * @param[in] codepoints Input codepoints.
 * @param[in] count Number of codepoints.
 * @param[out] output Destination buffer.
 */
static void input_to_utf8(const uint32_t *codepoints, size_t count, char *output) {
    size_t offset = 0U;

    for (size_t i = 0U; i < count; i++) {
        uint32_t cp = codepoints[i];
        if (cp <= 0x7FU) {
            output[offset++] = (char)cp;
        } else if (cp <= 0x7FFU) {
            output[offset++] = (char)(0xC0U | (cp >> 6));
            output[offset++] = (char)(0x80U | (cp & 0x3FU));
        } else if (cp <= 0xFFFFU) {
            output[offset++] = (char)(0xE0U | (cp >> 12));
            output[offset++] = (char)(0x80U | ((cp >> 6) & 0x3FU));
            output[offset++] = (char)(0x80U | (cp & 0x3FU));
        } else if (cp <= 0x10FFFFU) {
            output[offset++] = (char)(0xF0U | (cp >> 18));
            output[offset++] = (char)(0x80U | ((cp >> 12) & 0x3FU));
            output[offset++] = (char)(0x80U | ((cp >> 6) & 0x3FU));
            output[offset++] = (char)(0x80U | (cp & 0x3FU));
        } else {
            output[offset++] = '?';
        }
    }

    output[offset] = '\0';
}

/**
 * @brief Handles one translated keyboard event for the shell.
 * @param[in] event Translated HID input event.
 */
void shell_handle_event(const input_event_t *event) {
    if (!shell_is_running() || !event || event->hid.type != HID_EVENT_KEY_PRESS ||
        (event->key_action != INPUT_KEY_ACTION_PRESSED &&
         event->key_action != INPUT_KEY_ACTION_REPEATED)) {
        return;
    }

    uint32_t command[SHELL_INPUT_MAX];
    size_t command_length = 0U;
    bool submit = false;
    hid_keycode_t key = event->hid.key.keycode;

    irq_flags_t flags = shell_screen_lock();
    shell_screen_t *screen = shell_get_screen();

    if (key == KEY_LEFT) {
        if (screen->input_cursor > 0U) {
            screen->input_cursor--;
            screen->dirty = true;
        }
    } else if (key == KEY_RIGHT) {
        if (screen->input_cursor < screen->input_length) {
            screen->input_cursor++;
            screen->dirty = true;
        }
    } else if (key == KEY_HOME) {
        screen->input_cursor = 0U;
        screen->dirty = true;
    } else if (key == KEY_END) {
        screen->input_cursor = screen->input_length;
        screen->dirty = true;
    } else if (key == KEY_BACK) {
        input_backspace(screen);
    } else if (key == KEY_UP) {
        if (command_history_count > 0U && history_cursor > 0U) {
            history_cursor--;
            history_load(screen, history_cursor);
        }
    } else if (key == KEY_DOWN) {
        if (history_cursor < command_history_count) {
            history_cursor++;
            if (history_cursor == command_history_count) {
                screen->input_length = 0U;
                screen->input_cursor = 0U;
                screen->input[0] = 0U;
                screen->dirty = true;
            } else {
                history_load(screen, history_cursor);
            }
        }
    } else if (key == KEY_RETURN || key == KEY_NUMLOCK_RETURN) {
        if (event->key_action == INPUT_KEY_ACTION_PRESSED) {
            command_length = screen->input_length;
            memcpy(command, screen->input, command_length * sizeof(uint32_t));
            history_store(screen);
            screen->input_length = 0U;
            screen->input_cursor = 0U;
            screen->input[0] = 0U;
            screen->dirty = true;
            submit = true;
        }
    } else if (key != KEY_TAB && event->unicode_count > 0U &&
               event->unicode >= 0x20U && event->unicode != 0x7FU) {
        input_insert(screen, event->unicode);
        if (event->unicode_count > 1U && event->unicode_secondary >= 0x20U &&
            event->unicode_secondary != 0x7FU) {
            input_insert(screen, event->unicode_secondary);
        }
    }

    shell_screen_unlock(flags);

    if (submit) {
        char line[(SHELL_INPUT_MAX * 4U) + 1U];
        input_to_utf8(command, command_length, line);
        shell_write(SHELL_PROMPT);
        shell_writeln(line);
        if (command_length > 0U) {
            shell_execute_command(line);
        }
    }
}
