/**
 * @file shell.c
 * @brief Kernel shell state, terminal output and initialization.
 * @author friedrichOsDev
 */

#include "shell_internal.h"
#include "shell_renderer.h"
#include <lib/util/mem.h>
#include <lib/util/str.h>
#include <sched/process.h>
#include <sched/sched.h>
#include <sched/thread.h>
#include <shell/shell.h>

static volatile bool shell_running;
static bool shell_initialized;
static volatile bool shell_renderer_stopped = true;
static spinlock_t shell_lock = SPINLOCK_INIT;
static layer_t *shell_layer;
static shell_screen_t shell_screen;

/**
 * @brief Acquires the lock protecting the terminal state.
 * @return Interrupt flags to restore when releasing the lock.
 */
irq_flags_t shell_screen_lock(void) {
    return spinlock_acquire_irqsave(&shell_lock);
}

/**
 * @brief Releases the terminal state lock.
 * @param[in] flags Interrupt flags returned by shell_screen_lock().
 */
void shell_screen_unlock(irq_flags_t flags) {
    spinlock_release_irqrestore(&shell_lock, flags);
}

/**
 * @brief Returns the current terminal state.
 * @return Pointer to the shared terminal state.
 */
shell_screen_t *shell_get_screen(void) {
    return &shell_screen;
}

/**
 * @brief Returns the shell compositor layer.
 * @return Shell layer, or NULL before initialization.
 */
layer_t *shell_get_layer(void) {
    return shell_layer;
}

/**
 * @brief Reports whether the shell renderer should continue running.
 * @return True while the shell is active.
 */
bool shell_is_running(void) {
    return shell_running;
}

/**
 * @brief Marks the renderer thread as stopped.
 */
void shell_renderer_finished(void) {
    shell_renderer_stopped = true;
}

/**
 * @brief Starts a new terminal output line and scrolls when necessary.
 */
static void shell_newline_locked(void) {
    size_t max_output_rows = (shell_screen.visible_rows > 1U)
                                 ? shell_screen.visible_rows - 1U
                                 : 1U;

    if (shell_screen.line_count < max_output_rows) {
        shell_screen.line_count++;
        memset(&shell_screen.rows[shell_screen.line_count - 1U], 0, sizeof(shell_row_t));
    } else if (max_output_rows > 1U) {
        memmove(&shell_screen.rows[0], &shell_screen.rows[1],
                (max_output_rows - 1U) * sizeof(shell_row_t));
        memset(&shell_screen.rows[max_output_rows - 1U], 0, sizeof(shell_row_t));
    } else {
        memset(&shell_screen.rows[0], 0, sizeof(shell_row_t));
    }

    shell_screen.column = 0U;
    shell_screen.dirty = true;
}

/**
 * @brief Appends one UTF-8 sequence to the active output row.
 * @param[in] bytes UTF-8 bytes to append.
 * @param[in] length Number of bytes in the sequence.
 */
static void shell_append_sequence_locked(const char *bytes, size_t length) {
    if (shell_screen.columns == 0U || length == 0U || length >= SHELL_OUTPUT_WIDTH) {
        return;
    }

    if (shell_screen.column >= shell_screen.columns) {
        shell_newline_locked();
    }

    shell_row_t *row = &shell_screen.rows[shell_screen.line_count - 1U];
    size_t row_length = strlen(row->text);
    if (row_length + length >= SHELL_OUTPUT_WIDTH) {
        shell_newline_locked();
        row = &shell_screen.rows[shell_screen.line_count - 1U];
        row_length = 0U;
    }

    for (size_t i = 0U; i < length; i++) {
        row->text[row_length + i] = bytes[i];
    }
    row->text[row_length + length] = '\0';
    shell_screen.column++;
    shell_screen.dirty = true;
}

/**
 * @brief Writes a UTF-8 string into the terminal output buffer.
 * @param[in] text Null-terminated text.
 */
void shell_write(const char *text) {
    if (!text) {
        return;
    }

    irq_flags_t flags = shell_screen_lock();
    const char *cursor = text;

    while (*cursor != '\0') {
        if (*cursor == '\n') {
            shell_newline_locked();
            cursor++;
            continue;
        }
        if (*cursor == '\r') {
            cursor++;
            continue;
        }
        if (*cursor == '\t') {
            size_t spaces = 4U - (shell_screen.column % 4U);
            for (size_t i = 0U; i < spaces; i++) {
                shell_append_sequence_locked(" ", 1U);
            }
            cursor++;
            continue;
        }

        size_t length = 1U;
        uint8_t lead = (uint8_t)cursor[0];
        if ((lead & 0xE0U) == 0xC0U && cursor[1] != '\0') {
            length = 2U;
        } else if ((lead & 0xF0U) == 0xE0U && cursor[1] != '\0' && cursor[2] != '\0') {
            length = 3U;
        } else if ((lead & 0xF8U) == 0xF0U && cursor[1] != '\0' &&
                   cursor[2] != '\0' && cursor[3] != '\0') {
            length = 4U;
        }

        shell_append_sequence_locked(cursor, length);
        cursor += length;
    }

    shell_screen_unlock(flags);
}

/**
 * @brief Writes a Unicode codepoint to the terminal.
 * @param[in] codepoint Unicode codepoint to encode and write.
 */
void shell_writec(uint32_t codepoint) {
    char encoded[5] = {0};
    size_t length = 0U;

    if (codepoint <= 0x7FU) {
        encoded[0] = (char)codepoint;
        length = 1U;
    } else if (codepoint <= 0x7FFU) {
        encoded[0] = (char)(0xC0U | (codepoint >> 6));
        encoded[1] = (char)(0x80U | (codepoint & 0x3FU));
        length = 2U;
    } else if (codepoint <= 0xFFFFU) {
        encoded[0] = (char)(0xE0U | (codepoint >> 12));
        encoded[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        encoded[2] = (char)(0x80U | (codepoint & 0x3FU));
        length = 3U;
    } else if (codepoint <= 0x10FFFFU) {
        encoded[0] = (char)(0xF0U | (codepoint >> 18));
        encoded[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
        encoded[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        encoded[3] = (char)(0x80U | (codepoint & 0x3FU));
        length = 4U;
    } else {
        encoded[0] = '?';
        length = 1U;
    }

    encoded[length] = '\0';
    shell_write(encoded);
}

/**
 * @brief Writes a string followed by a newline.
 * @param[in] text Null-terminated text.
 */
void shell_writeln(const char *text) {
    irq_flags_t flags = shell_screen_lock();
    const char *cursor = text ? text : "";

    while (*cursor != '\0') {
        if (*cursor == '\n') {
            shell_newline_locked();
            cursor++;
            continue;
        }
        if (*cursor == '\r') {
            cursor++;
            continue;
        }

        size_t length = 1U;
        uint8_t lead = (uint8_t)cursor[0];
        if ((lead & 0xE0U) == 0xC0U && cursor[1] != '\0') {
            length = 2U;
        } else if ((lead & 0xF0U) == 0xE0U && cursor[1] != '\0' && cursor[2] != '\0') {
            length = 3U;
        } else if ((lead & 0xF8U) == 0xF0U && cursor[1] != '\0' &&
                   cursor[2] != '\0' && cursor[3] != '\0') {
            length = 4U;
        }
        shell_append_sequence_locked(cursor, length);
        cursor += length;
    }

    shell_newline_locked();
    shell_screen_unlock(flags);
}

/**
 * @brief Clears all terminal output and restores an empty prompt line.
 */
void shell_clear(void) {
    irq_flags_t flags = shell_screen_lock();
    memset(shell_screen.rows, 0, sizeof(shell_screen.rows));
    shell_screen.line_count = 1U;
    shell_screen.column = 0U;
    shell_screen.dirty = true;
    shell_screen_unlock(flags);
}

/**
 * @brief Initializes the fullscreen terminal layer and renderer thread.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_init(void) {
    if (shell_initialized) {
        return E_NOTAVAIL;
    }

    rect_size_t screen = compositor_get_size();
    if (screen.width < 32U || screen.height < 48U) {
        return E_NOINIT;
    }

    memset(&shell_screen, 0, sizeof(shell_screen));
    shell_screen.visible_rows = (size_t)((screen.height - 16U) / 16U);
    if (shell_screen.visible_rows > SHELL_OUTPUT_LINES) {
        shell_screen.visible_rows = SHELL_OUTPUT_LINES;
    }
    if (shell_screen.visible_rows < 2U) {
        shell_screen.visible_rows = 2U;
    }

    shell_screen.columns = (size_t)((screen.width - 16U) / 8U);
    if (shell_screen.columns > SHELL_OUTPUT_COLUMNS) {
        shell_screen.columns = SHELL_OUTPUT_COLUMNS;
    }
    if (shell_screen.columns == 0U) {
        return E_NOINIT;
    }

    shell_screen.line_count = 1U;
    shell_screen.dirty = true;

    kerr_t err = layer_create(screen, 1000, &shell_layer);
    if (k_failed(err)) {
        shell_layer = NULL;
        return err;
    }

    shell_running = true;
    shell_renderer_stopped = false;

    thread_t *renderer_thread = NULL;
    err = thread_create(proc_get_kernel_proc(), shell_render_thread, NULL,
                        "shell renderer", &renderer_thread);
    if (k_failed(err)) {
        shell_running = false;
        shell_renderer_stopped = true;
        (void)layer_destroy(shell_layer);
        shell_layer = NULL;
        return err;
    }

    shell_initialized = true;
    log_info("kernel shell initialized");
    return E_OK;
}

/**
 * @brief Stops the shell renderer and releases the terminal layer.
 */
void shell_deinit(void) {
    if (!shell_initialized) {
        return;
    }

    shell_running = false;
    while (!shell_renderer_stopped) {
        thread_yield();
    }

    if (shell_layer) {
        (void)layer_destroy(shell_layer);
        shell_layer = NULL;
    }

    shell_initialized = false;
}
