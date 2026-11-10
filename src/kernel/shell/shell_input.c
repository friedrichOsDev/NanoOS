/**
 * @file shell_input.c
 * @brief Kernel shell input and terminal buffer.
 * @author friedrichOsDev
 */

#include "shell_input.h"
#include "shell_commands.h"
#include <drivers/hid/input.h>
#include <lib/util/str.h>
#include <sched/sched.h>
#include <shell/shell.h>

#define SHELL_INPUT_CAPACITY 256U
#define SHELL_HISTORY_CAPACITY 16U
#define SHELL_LINE_CAPACITY 256U
#define SHELL_OUTPUT_CAPACITY 128U
#define SHELL_DEFAULT_COLUMNS 80U

static char input_buffer[SHELL_INPUT_CAPACITY];
static size_t input_length;
static size_t input_cursor;

static char command_history[SHELL_HISTORY_CAPACITY][SHELL_INPUT_CAPACITY];
static size_t history_head;
static size_t history_count;
static int32_t history_position = -1;
static char history_scratch[SHELL_INPUT_CAPACITY];

static char output_lines[SHELL_OUTPUT_CAPACITY][SHELL_LINE_CAPACITY];
static size_t output_head;
static size_t output_count;
static size_t output_length;
static size_t terminal_columns = SHELL_DEFAULT_COLUMNS;

/** @brief Start a fresh terminal output line, advancing the bounded scrollback ring. */
static void terminal_newline(void) {
    output_head = (output_head + 1U) % SHELL_OUTPUT_CAPACITY;
    output_lines[output_head][0] = '\0';
    output_length = 0U;
    if (output_count < SHELL_OUTPUT_CAPACITY) {
        output_count++;
    }
}

/** @brief Return the UTF-8 byte length of a character beginning at text. */
static size_t utf8_sequence_length(const char *text) {
    const uint8_t first = (uint8_t)text[0];
    if (first < 0x80U) {
        return 1U;
    }
    if ((first & 0xE0U) == 0xC0U) {
        return 2U;
    }
    if ((first & 0xF0U) == 0xE0U) {
        return 3U;
    }
    if ((first & 0xF8U) == 0xF0U) {
        return 4U;
    }
    return 1U;
}

/** @brief Append UTF-8 text to the terminal, wrapping at the active screen width. */
void shell_console_write(const char *text) {
    if (!text) {
        return;
    }

    size_t offset = 0U;
    while (text[offset] != '\0') {
        const char current = text[offset];
        if (current == '\n') {
            terminal_newline();
            offset++;
            continue;
        }
        if (current == '\r') {
            offset++;
            continue;
        }
        if (current == '\t') {
            for (size_t i = 0U; i < 4U; i++) {
                shell_console_write(" ");
            }
            offset++;
            continue;
        }

        size_t sequence_length = utf8_sequence_length(&text[offset]);
        size_t remaining = strlen(&text[offset]);
        if (sequence_length > remaining) {
            sequence_length = 1U;
        }

        if (output_length + sequence_length >= terminal_columns ||
            output_length + sequence_length >= SHELL_LINE_CAPACITY) {
            terminal_newline();
        }

        for (size_t i = 0U; i < sequence_length; i++) {
            output_lines[output_head][output_length] = text[offset + i];
            output_length++;
        }
        output_lines[output_head][output_length] = '\0';
        offset += sequence_length;
    }
}

/** @brief Append text followed by a newline to the terminal. */
void shell_console_write_line(const char *text) {
    shell_console_write(text);
    shell_console_write("\n");
}

/** @brief Reset terminal scrollback without changing the current input or history. */
void shell_console_clear(void) {
    for (size_t i = 0U; i < SHELL_OUTPUT_CAPACITY; i++) {
        output_lines[i][0] = '\0';
    }
    output_head = 0U;
    output_count = 1U;
    output_length = 0U;
}

/** @brief Initialize the terminal buffers and set the wrapping width in characters. */
void shell_console_init(size_t columns) {
    if (columns == 0U) {
        columns = SHELL_DEFAULT_COLUMNS;
    }
    if (columns >= SHELL_LINE_CAPACITY) {
        columns = SHELL_LINE_CAPACITY - 1U;
    }
    terminal_columns = columns;

    input_buffer[0] = '\0';
    input_length = 0U;
    input_cursor = 0U;
    history_head = 0U;
    history_count = 0U;
    history_position = -1;
    history_scratch[0] = '\0';
    shell_console_clear();
}

/** @brief Return the number of currently retained output lines, oldest first. */
size_t shell_console_line_count(void) {
    return output_count;
}

/** @brief Return a retained output line by its zero-based chronological index. */
const char *shell_console_line_at(size_t index) {
    if (index >= output_count) {
        return NULL;
    }
    size_t first = (output_head + SHELL_OUTPUT_CAPACITY - output_count + 1U) % SHELL_OUTPUT_CAPACITY;
    size_t slot = (first + index) % SHELL_OUTPUT_CAPACITY;
    return output_lines[slot];
}

/** @brief Return the current editable command buffer. */
const char *shell_console_input(void) {
    return input_buffer;
}

/** @brief Return the cursor's byte offset in the current UTF-8 command buffer. */
size_t shell_console_cursor(void) {
    return input_cursor;
}

/** @brief Find the beginning of the UTF-8 character immediately before position. */
static size_t previous_character(size_t position) {
    if (position == 0U) {
        return 0U;
    }
    position--;
    while (position > 0U && (((uint8_t)input_buffer[position] & 0xC0U) == 0x80U)) {
        position--;
    }
    return position;
}

/** @brief Find the byte immediately after the UTF-8 character at position. */
static size_t next_character(size_t position) {
    if (position >= input_length) {
        return input_length;
    }
    size_t next = position + utf8_sequence_length(&input_buffer[position]);
    return next > input_length ? input_length : next;
}

/** @brief Replace the editable command with a stored history entry. */
static void history_load(const char *entry) {
    strncpy(input_buffer, entry, SHELL_INPUT_CAPACITY - 1U);
    input_buffer[SHELL_INPUT_CAPACITY - 1U] = '\0';
    input_length = strlen(input_buffer);
    input_cursor = input_length;
}

/** @brief Store a non-empty command in the bounded history ring. */
static void history_store(const char *command) {
    if (!command || command[0] == '\0') {
        return;
    }
    if (history_count > 0U) {
        size_t newest = (history_head + SHELL_HISTORY_CAPACITY - 1U) % SHELL_HISTORY_CAPACITY;
        if (strcmp(command_history[newest], command) == 0) {
            return;
        }
    }

    strncpy(command_history[history_head], command, SHELL_INPUT_CAPACITY - 1U);
    command_history[history_head][SHELL_INPUT_CAPACITY - 1U] = '\0';
    history_head = (history_head + 1U) % SHELL_HISTORY_CAPACITY;
    if (history_count < SHELL_HISTORY_CAPACITY) {
        history_count++;
    }
}

/** @brief Move to the previous command in history, preserving the current edit. */
static void history_previous(void) {
    if (history_count == 0U) {
        return;
    }
    if (history_position < 0) {
        strncpy(history_scratch, input_buffer, SHELL_INPUT_CAPACITY - 1U);
        history_scratch[SHELL_INPUT_CAPACITY - 1U] = '\0';
        history_position = (int32_t)(history_count - 1U);
    } else if (history_position > 0) {
        history_position--;
    } else {
        return;
    }

    size_t oldest = (history_head + SHELL_HISTORY_CAPACITY - history_count) % SHELL_HISTORY_CAPACITY;
    size_t slot = (oldest + (size_t)history_position) % SHELL_HISTORY_CAPACITY;
    history_load(command_history[slot]);
}

/** @brief Move to the next command in history or restore the saved edit. */
static void history_next(void) {
    if (history_position < 0) {
        return;
    }
    if ((size_t)history_position + 1U < history_count) {
        history_position++;
        size_t oldest = (history_head + SHELL_HISTORY_CAPACITY - history_count) % SHELL_HISTORY_CAPACITY;
        size_t slot = (oldest + (size_t)history_position) % SHELL_HISTORY_CAPACITY;
        history_load(command_history[slot]);
    } else {
        history_position = -1;
        history_load(history_scratch);
    }
}

/** @brief Encode a Unicode codepoint as UTF-8 and insert it at the current cursor. */
static void input_insert_codepoint(uint32_t codepoint) {
    char encoded[4];
    size_t count;

    if (codepoint < 0x80U) {
        encoded[0] = (char)codepoint;
        count = 1U;
    } else if (codepoint < 0x800U) {
        encoded[0] = (char)(0xC0U | (codepoint >> 6));
        encoded[1] = (char)(0x80U | (codepoint & 0x3FU));
        count = 2U;
    } else if (codepoint < 0x10000U &&
               !(codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
        encoded[0] = (char)(0xE0U | (codepoint >> 12));
        encoded[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        encoded[2] = (char)(0x80U | (codepoint & 0x3FU));
        count = 3U;
    } else if (codepoint <= 0x10FFFFU) {
        encoded[0] = (char)(0xF0U | (codepoint >> 18));
        encoded[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
        encoded[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        encoded[3] = (char)(0x80U | (codepoint & 0x3FU));
        count = 4U;
    } else {
        return;
    }

    if (input_length + count >= SHELL_INPUT_CAPACITY) {
        return;
    }

    for (size_t i = input_length + 1U; i > input_cursor; i--) {
        input_buffer[i + count - 1U] = input_buffer[i - 1U];
    }
    for (size_t i = 0U; i < count; i++) {
        input_buffer[input_cursor + i] = encoded[i];
    }
    input_length += count;
    input_cursor += count;
    history_position = -1;
}

/** @brief Delete the UTF-8 character at the cursor. */
static void input_delete_at_cursor(void) {
    if (input_cursor >= input_length) {
        return;
    }
    size_t next = next_character(input_cursor);
    size_t count = next - input_cursor;
    for (size_t i = input_cursor; i + count <= input_length; i++) {
        input_buffer[i] = input_buffer[i + count];
    }
    input_length -= count;
}

/** @brief Submit the current command and start a fresh input line. */
static void input_submit(void) {
    char command[SHELL_INPUT_CAPACITY];
    strncpy(command, input_buffer, SHELL_INPUT_CAPACITY - 1U);
    command[SHELL_INPUT_CAPACITY - 1U] = '\0';

    shell_console_write("friedrichOsDev@nanoos> ");
    shell_console_write(command);
    shell_console_write("\n");

    history_store(command);
    input_buffer[0] = '\0';
    input_length = 0U;
    input_cursor = 0U;
    history_position = -1;

    if (command[0] != '\0') {
        shell_execute_command(command);
    }
}

/** @brief Handle one translated keyboard event. */
static void shell_handle_event(const input_event_t *event) {
    if (!event || event->hid.type != HID_EVENT_KEY_PRESS ||
        (event->key_action != INPUT_KEY_ACTION_PRESSED &&
         event->key_action != INPUT_KEY_ACTION_REPEATED)) {
        return;
    }

    switch (event->hid.key.keycode) {
    case KEY_RETURN:
    case KEY_NUMLOCK_RETURN:
        input_submit();
        return;
    case KEY_BACK:
        if (input_cursor > 0U) {
            size_t previous = previous_character(input_cursor);
            size_t count = input_cursor - previous;
            for (size_t i = previous; i + count <= input_length; i++) {
                input_buffer[i] = input_buffer[i + count];
            }
            input_length -= count;
            input_cursor = previous;
        }
        return;
    case KEY_DELETE:
        input_delete_at_cursor();
        return;
    case KEY_LEFT:
        input_cursor = previous_character(input_cursor);
        return;
    case KEY_RIGHT:
        input_cursor = next_character(input_cursor);
        return;
    case KEY_HOME:
        input_cursor = 0U;
        return;
    case KEY_END:
        input_cursor = input_length;
        return;
    case KEY_UP:
        history_previous();
        return;
    case KEY_DOWN:
        history_next();
        return;
    default:
        break;
    }

    if (event->unicode_count > 0U && event->unicode >= 0x20U && event->unicode != 0x7FU) {
        input_insert_codepoint(event->unicode);
    }
    if (event->unicode_count > 1U &&
        event->unicode_secondary >= 0x20U && event->unicode_secondary != 0x7FU) {
        input_insert_codepoint(event->unicode_secondary);
    }
}

/**
 * @brief Poll input events and update the terminal until the shell is stopped.
 *
 * @param[in] arg Unused argument.
 */
void shell_input_thread(void *arg) {
    (void)arg;

    while (shell_is_running()) {
        input_event_t event;
        kerr_t err = input_poll_event(&event);

        if (err == E_NOTAVAIL) {
            shell_render_update();
            thread_sleep_ms(10U);
            continue;
        }
        if (k_failed(err)) {
            log_warn("shell input polling failed (%s)", kerr_str(err));
            thread_sleep_ms(10U);
            continue;
        }

        shell_handle_event(&event);
        shell_render_update();
    }

    shell_renderer_deinit();
    thread_exit();
}
