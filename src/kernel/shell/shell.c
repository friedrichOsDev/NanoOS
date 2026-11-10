/**
 * @file shell.c
 * @brief Kernel shell state, terminal output and command dispatch.
 * @author friedrichOsDev
 */

#include "shell_internal.h"
#include "shell_input.h"
#include "shell_renderer.h"
#include <arch/arch.h>
#include <drivers/gfx/gfx.h>
#include <kernel/sync.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <sched/process.h>
#include <sched/sched.h>
#include <sched/thread.h>
#include <shell/shell.h>

#define SHELL_PROMPT "friedrichOsDev@nanoos> "

static bool shell_running = false;
static bool shell_initialized = false;
static spinlock_t shell_lock = SPINLOCK_INIT;
static vmm_space_t shell_space;
static process_t *shell_process = NULL;
static layer_t *shell_layer = NULL;

static char output_lines[SHELL_OUTPUT_LINES][SHELL_OUTPUT_WIDTH];
static size_t output_count = 1U;
static size_t output_column = 0U;
static char input_line[SHELL_INPUT_MAX];
static size_t input_length = 0U;
static size_t input_cursor = 0U;
static char command_history[SHELL_HISTORY_COUNT][SHELL_INPUT_MAX];
static size_t history_count = 0U;
static size_t history_next = 0U;
static size_t history_view = 0U;

static void shell_newline_locked(void) {
    if (output_count < SHELL_OUTPUT_LINES) {
        output_count++;
    } else {
        for (size_t row = 1U; row < SHELL_OUTPUT_LINES; row++) {
            for (size_t col = 0U; col < SHELL_OUTPUT_WIDTH; col++) {
                output_lines[row - 1U][col] = output_lines[row][col];
            }
        }
        for (size_t col = 0U; col < SHELL_OUTPUT_WIDTH; col++) {
            output_lines[SHELL_OUTPUT_LINES - 1U][col] = '\0';
        }
    }
    output_column = 0U;
}

/** @brief Appends UTF-8 text while preserving character boundaries at line wraps. */
static void shell_write_locked(const char *text) {
    while (*text != '\0') {
        if (*text == '\r') {
            text++;
            continue;
        }
        if (*text == '\n') {
            shell_newline_locked();
            text++;
            continue;
        }

        size_t bytes = 1U;
        uint8_t first = (uint8_t)*text;
        if ((first & 0xE0U) == 0xC0U) bytes = 2U;
        else if ((first & 0xF0U) == 0xE0U) bytes = 3U;
        else if ((first & 0xF8U) == 0xF0U) bytes = 4U;

        size_t remaining = strlen(text);
        if (bytes > remaining) bytes = 1U;
        if (output_column + bytes >= SHELL_OUTPUT_WIDTH) {
            shell_newline_locked();
        }
        for (size_t i = 0U; i < bytes; i++) {
            output_lines[output_count - 1U][output_column++] = text[i];
        }
        output_lines[output_count - 1U][output_column] = '\0';
        text += bytes;
    }
}

void shell_write(const char *text) {
    if (!text) return;
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_lock);
    shell_write_locked(text);
    spinlock_release_irqrestore(&shell_lock, flags);
}

void shell_writeln(const char *text) {
    shell_write(text ? text : "");
    shell_write("\n");
}

void shell_write_u64(uint64_t value) {
    char digits[21];
    size_t count = 0U;
    do {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U && count < sizeof(digits));

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_lock);
    while (count > 0U) {
        shell_write_locked((char[]){digits[--count], '\0'});
    }
    spinlock_release_irqrestore(&shell_lock, flags);
}

void shell_clear(void) {
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_lock);
    for (size_t row = 0U; row < SHELL_OUTPUT_LINES; row++) {
        for (size_t col = 0U; col < SHELL_OUTPUT_WIDTH; col++) {
            output_lines[row][col] = '\0';
        }
    }
    output_count = 1U;
    output_column = 0U;
    spinlock_release_irqrestore(&shell_lock, flags);
}

/** @brief Returns the byte offset of the previous UTF-8 character. */
static size_t utf8_previous(const char *text, size_t offset) {
    if (offset == 0U) return 0U;
    offset--;
    while (offset > 0U && (((uint8_t)text[offset] & 0xC0U) == 0x80U)) {
        offset--;
    }
    return offset;
}

/** @brief Returns the byte offset immediately after the UTF-8 character at offset. */
static size_t utf8_next(const char *text, size_t length, size_t offset) {
    if (offset >= length) return length;
    offset++;
    while (offset < length && (((uint8_t)text[offset] & 0xC0U) == 0x80U)) {
        offset++;
    }
    return offset;
}

/** @brief Encodes a Unicode codepoint into UTF-8. */
static size_t utf8_encode(uint32_t codepoint, char out[4]) {
    if (codepoint <= 0x7FU) {
        out[0] = (char)codepoint;
        return 1U;
    }
    if (codepoint <= 0x7FFU) {
        out[0] = (char)(0xC0U | (codepoint >> 6));
        out[1] = (char)(0x80U | (codepoint & 0x3FU));
        return 2U;
    }
    if (codepoint <= 0xFFFFU && !(codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
        out[0] = (char)(0xE0U | (codepoint >> 12));
        out[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        out[2] = (char)(0x80U | (codepoint & 0x3FU));
        return 3U;
    }
    if (codepoint <= 0x10FFFFU) {
        out[0] = (char)(0xF0U | (codepoint >> 18));
        out[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
        out[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        out[3] = (char)(0x80U | (codepoint & 0x3FU));
        return 4U;
    }
    return 0U;
}

/** @brief Inserts one UTF-8 character at the current cursor position. */
static void shell_insert_codepoint_locked(uint32_t codepoint) {
    char encoded[4];
    size_t bytes = utf8_encode(codepoint, encoded);
    if (bytes == 0U || input_length + bytes >= SHELL_INPUT_MAX) {
        return;
    }
    for (size_t i = input_length; i > input_cursor; i--) {
        input_line[i + bytes - 1U] = input_line[i - 1U];
    }
    for (size_t i = 0U; i < bytes; i++) {
        input_line[input_cursor + i] = encoded[i];
    }
    input_length += bytes;
    input_cursor += bytes;
    input_line[input_length] = '\0';
    history_view = history_count;
}

/** @brief Restores a command from the circular history buffer. */
static void shell_history_move_locked(bool older) {
    if (history_count == 0U) return;
    if (older) {
        if (history_view == 0U) return;
        history_view--;
    } else {
        if (history_view >= history_count) return;
        history_view++;
    }

    if (history_view == history_count) {
        input_length = 0U;
        input_cursor = 0U;
        input_line[0] = '\0';
        return;
    }

    size_t oldest = (history_next + SHELL_HISTORY_COUNT - history_count) % SHELL_HISTORY_COUNT;
    size_t index = (oldest + history_view) % SHELL_HISTORY_COUNT;
    size_t length = strlen(command_history[index]);
    if (length >= SHELL_INPUT_MAX) length = SHELL_INPUT_MAX - 1U;
    for (size_t i = 0U; i < length; i++) input_line[i] = command_history[index][i];
    input_line[length] = '\0';
    input_length = length;
    input_cursor = length;
}

void shell_handle_event(const input_event_t *event) {
    if (!event || event->hid.type != HID_EVENT_KEY_PRESS ||
        (event->key_action != INPUT_KEY_ACTION_PRESSED &&
         event->key_action != INPUT_KEY_ACTION_REPEATED)) {
        return;
    }

    char submitted[SHELL_INPUT_MAX];
    bool execute = false;
    submitted[0] = '\0';

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_lock);
    hid_keycode_t key = event->hid.key.keycode;

    switch (key) {
    case KEY_LEFT:
        input_cursor = utf8_previous(input_line, input_cursor);
        break;
    case KEY_RIGHT:
        input_cursor = utf8_next(input_line, input_length, input_cursor);
        break;
    case KEY_HOME:
        input_cursor = 0U;
        break;
    case KEY_END:
        input_cursor = input_length;
        break;
    case KEY_BACK:
        if (input_cursor > 0U) {
            size_t previous = utf8_previous(input_line, input_cursor);
            size_t removed = input_cursor - previous;
            for (size_t i = input_cursor; i <= input_length; i++) {
                input_line[i - removed] = input_line[i];
            }
            input_length -= removed;
            input_cursor = previous;
        }
        break;
    case KEY_DELETE:
        if (input_cursor < input_length) {
            size_t next = utf8_next(input_line, input_length, input_cursor);
            size_t removed = next - input_cursor;
            for (size_t i = next; i <= input_length; i++) {
                input_line[i - removed] = input_line[i];
            }
            input_length -= removed;
        }
        break;
    case KEY_UP:
        shell_history_move_locked(true);
        break;
    case KEY_DOWN:
        shell_history_move_locked(false);
        break;
    case KEY_RETURN:
    case KEY_NUMLOCK_RETURN:
        for (size_t i = 0U; i <= input_length; i++) submitted[i] = input_line[i];
        if (input_length > 0U) {
            size_t copy_len = input_length;
            if (copy_len >= SHELL_INPUT_MAX) copy_len = SHELL_INPUT_MAX - 1U;
            for (size_t i = 0U; i < copy_len; i++) {
                command_history[history_next][i] = input_line[i];
            }
            command_history[history_next][copy_len] = '\0';
            history_next = (history_next + 1U) % SHELL_HISTORY_COUNT;
            if (history_count < SHELL_HISTORY_COUNT) history_count++;
        }
        input_length = 0U;
        input_cursor = 0U;
        input_line[0] = '\0';
        history_view = history_count;
        execute = true;
        break;
    default:
        if (event->unicode_count > 0U && event->unicode >= 0x20U &&
            event->unicode != 0x7FU) {
            shell_insert_codepoint_locked(event->unicode);
        }
        if (event->unicode_count > 1U && event->unicode_secondary >= 0x20U &&
            event->unicode_secondary != 0x7FU) {
            shell_insert_codepoint_locked(event->unicode_secondary);
        }
        break;
    }
    spinlock_release_irqrestore(&shell_lock, flags);

    if (execute) {
        shell_write(SHELL_PROMPT);
        shell_writeln(submitted);
        shell_execute_command(submitted);
    }
}

/**
 * @brief Copies a consistent terminal snapshot for the renderer.
 * @param[out] lines Destination output-line array.
 * @param[out] line_count Number of valid output lines.
 * @param[out] input Destination editable command buffer.
 * @param[out] input_bytes Number of bytes in the input buffer.
 * @param[out] cursor_byte Cursor byte offset in the input buffer.
 */
void shell_get_snapshot(char lines[SHELL_OUTPUT_LINES][SHELL_OUTPUT_WIDTH],
                        size_t *line_count,
                        char input[SHELL_INPUT_MAX],
                        size_t *input_bytes,
                        size_t *cursor_byte) {
    if (!lines || !line_count || !input || !input_bytes || !cursor_byte) return;
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_lock);
    *line_count = output_count;
    *input_bytes = input_length;
    *cursor_byte = input_cursor;
    for (size_t row = 0U; row < SHELL_OUTPUT_LINES; row++) {
        for (size_t col = 0U; col < SHELL_OUTPUT_WIDTH; col++) {
            lines[row][col] = output_lines[row][col];
        }
    }
    for (size_t col = 0U; col < SHELL_INPUT_MAX; col++) {
        input[col] = input_line[col];
    }
    spinlock_release_irqrestore(&shell_lock, flags);
}

/** @brief Returns the shell's compositor layer. */
layer_t *shell_get_layer(void) {
    return shell_layer;
}

void shell_request_shutdown(void) {
    acpi_power_off();
    shell_writeln("ACPI shutdown request returned.");
}

/**
 * @brief Initializes the terminal, its process and its input/render threads.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_init(void) {
    if (shell_initialized) return E_NOTAVAIL;

    rect_size_t screen = compositor_get_size();
    if (screen.width == 0U || screen.height < 48U ||
        screen.width > (uint64_t)INT64_MAX || screen.height > (uint64_t)INT64_MAX) {
        return E_NOINIT;
    }

    for (size_t row = 0U; row < SHELL_OUTPUT_LINES; row++) {
        output_lines[row][0] = '\0';
    }
    output_count = 1U;
    output_column = 0U;
    input_length = 0U;
    input_cursor = 0U;
    history_count = 0U;
    history_next = 0U;
    history_view = 0U;

    K_TRY(vmm_space_create(&shell_space));
    kerr_t err = proc_create("shell", &shell_space, &shell_process);
    if (k_failed(err)) {
        vmm_space_destroy(&shell_space);
        return err;
    }

    err = layer_create(screen, 1000, &shell_layer);
    if (k_failed(err)) {
        proc_unregister(shell_process);
        kfree((virt_addr_t)shell_process);
        shell_process = NULL;
        vmm_space_destroy(&shell_space);
        return err;
    }

    shell_running = true;
    thread_t *render_thread = NULL;
    err = thread_create(shell_process, shell_render_thread, NULL, "shell renderer", &render_thread);
    if (k_failed(err)) {
        shell_running = false;
        layer_destroy(shell_layer);
        shell_layer = NULL;
        proc_unregister(shell_process);
        kfree((virt_addr_t)shell_process);
        shell_process = NULL;
        vmm_space_destroy(&shell_space);
        return err;
    }

    thread_t *input_thread = NULL;
    err = thread_create(shell_process, shell_input_thread, NULL, "shell input", &input_thread);
    if (k_failed(err)) {
        shell_running = false;
        return err;
    }

    shell_initialized = true;
    log_info("kernel shell initialized");
    return E_OK;
}

/** @brief Reports whether the shell input and renderer threads should run. */
bool shell_is_running(void) {
    return shell_running;
}

/** @brief Stops the shell worker loops. */
void shell_deinit(void) {
    shell_running = false;
}
