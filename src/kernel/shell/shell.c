/**
 * @file shell.c
 * @brief Interactive character input for the kernel shell.
 * @author friedrichOsDev
 */

#include <drivers/gfx/compositor.h>
#include <drivers/hid/input.h>
#include <kernel/sync.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <sched/process.h>
#include <sched/sched.h>
#include <sched/thread.h>
#include <shell/renderer.h>
#include <shell/screen.h>
#include <shell/shell.h>

#define SHELL_HEADER_ROW 0U
#define SHELL_INFO_ROW 1U
#define SHELL_STATUS_ROW 2U
#define SHELL_PROMPT_ROW 4U
#define SHELL_RENDER_INTERVAL_MS 16U
#define SHELL_INPUT_MAX_CODEPOINTS 4096U

static const char shell_prompt[] = "nanoos> ";

static shell_screen_t shell_screen;
static shell_screen_t shell_render_screen;
static uint32_t *shell_history_cells;
static uint32_t *shell_input_buffer;
static size_t shell_input_length;
static size_t shell_input_cursor;
static size_t shell_input_start_index;
static size_t shell_preferred_column;
static bool shell_preferred_column_valid;
static spinlock_t shell_lock = SPINLOCK_INIT;
static bool shell_initialized;
static bool shell_dirty;

/**
 * @brief Copy committed screen contents and overlay the visible portion of active input.
 *
 * @param[out] cursor_index Current cursor position in the visible screen.
 * @return E_OK on success or an error code when the screen state is invalid.
 */
static kerr_t shell_rebuild_screen(size_t *cursor_index) {
    if (!cursor_index || !shell_screen.cells || !shell_history_cells ||
        !shell_input_buffer || shell_screen.columns == 0U ||
        shell_screen.rows == 0U ||
        shell_input_cursor > shell_input_length ||
        shell_input_length > SHELL_INPUT_MAX_CODEPOINTS ||
        shell_input_start_index > SIZE_MAX - shell_input_length) {
        return E_INVAL;
    }

    size_t columns = shell_screen.columns;
    size_t rows = shell_screen.rows;
    cell_count = columns * rows;
    size_t absolute_cursor = shell_input_start_index + shell_input_cursor;
    size_t cursor_row = absolute_cursor / columns;
    size_t view_start_row = (cursor_row >= rows) ? cursor_row - rows + 1U : 0U;
    size_t view_start_cell = view_start_row * columns;

    memcpy(shell_screen.cells, shell_history_cells,
           cell_count * sizeof(uint32_t));

    if (view_start_row > 0U) {
        size_t removed_cells = view_start_cell;
        memmove(shell_screen.cells, shell_screen.cells + removed_cells,
                (cell_count - removed_cells) * sizeof(uint32_t));
        memset32(shell_screen.cells + cell_count - removed_cells, 0U,
                 removed_cells);
    }

    for (size_t i = 0U; i < shell_input_length; i++) {
        size_t absolute_index = shell_input_start_index + i;
        if (absolute_index >= view_start_cell &&
            absolute_index - view_start_cell < cell_count) {
            shell_screen.cells[absolute_index - view_start_cell] =
                shell_input_buffer[i];
        }
    }

    *cursor_index = absolute_cursor - view_start_cell;
    if (*cursor_index >= cell_count) {
        return E_BADSIZE;
    }

    return E_OK;
}

/**
 * @brief Scroll committed shell history upward by a number of rows.
 *
 * @param[in] rows Number of rows to remove from the top of the screen.
 */
static void shell_history_scroll(size_t rows) {
    size_t columns = shell_screen.columns;
    size_t total_cells = columns * shell_screen.rows;

    if (rows == 0U || rows >= shell_screen.rows) {
        return;
    }

    size_t removed_cells = rows * columns;
    memmove(shell_history_cells, shell_history_cells + removed_cells,
            (total_cells - removed_cells) * sizeof(uint32_t));
    memset32(shell_history_cells + total_cells - removed_cells, 0U,
             removed_cells);
}

/**
 * @brief Draw the shell prompt into committed screen contents.
 *
 * @param[in] start_index First cell of the prompt.
 * @return E_OK on success or E_BADSIZE when the prompt does not fit.
 */
static kerr_t shell_history_write_prompt(size_t start_index) {
    size_t prompt_length = sizeof(shell_prompt) - 1U;
    size_t cell_count = shell_screen.columns * shell_screen.rows;

    if (start_index >= cell_count ||
        prompt_length >= cell_count - start_index) {
        return E_BADSIZE;
    }

    for (size_t i = 0U; i < prompt_length; i++) {
        shell_history_cells[start_index + i] = (uint32_t)shell_prompt[i];
    }

    shell_input_start_index = start_index + prompt_length;
    return E_OK;
}

/**
 * @brief Insert one printable Unicode codepoint at the logical cursor position.
 *
 * @param[in] codepoint Unicode scalar value to insert.
 */
static void shell_insert_codepoint(uint32_t codepoint) {
    if (codepoint < 0x20U || codepoint == 0x7FU ||
        codepoint > 0x10FFFFU ||
        (codepoint >= 0xD800U && codepoint <= 0xDFFFU) ||
        shell_input_length >= SHELL_INPUT_MAX_CODEPOINTS) {
        return;
    }

    size_t trailing = shell_input_length - shell_input_cursor;
    if (trailing > 0U) {
        memmove(&shell_input_buffer[shell_input_cursor + 1U],
                &shell_input_buffer[shell_input_cursor],
                trailing * sizeof(uint32_t));
    }

    shell_input_buffer[shell_input_cursor] = codepoint;
    shell_input_cursor++;
    shell_input_length++;
    shell_preferred_column_valid = false;
}

/** @brief Insert a translated tab as four spaces. */
static void shell_insert_tab() {
    for (size_t i = 0U; i < 4U; i++) {
        shell_insert_codepoint((uint32_t)' ');
    }
}

/** @brief Remove the codepoint immediately before the logical cursor. */
static void shell_backspace() {
    if (shell_input_cursor == 0U) {
        return;
    }

    size_t trailing = shell_input_length - shell_input_cursor;
    if (trailing > 0U) {
        memmove(&shell_input_buffer[shell_input_cursor - 1U],
                &shell_input_buffer[shell_input_cursor],
                trailing * sizeof(uint32_t));
    }

    shell_input_cursor--;
    shell_input_length--;
    shell_input_buffer[shell_input_length] = 0U;
    shell_preferred_column_valid = false;
}

/** @brief Delete the codepoint at the logical cursor without moving the cursor. */
static void shell_delete() {
    if (shell_input_cursor >= shell_input_length) {
        return;
    }

    size_t trailing = shell_input_length - shell_input_cursor - 1U;
    if (trailing > 0U) {
        memmove(&shell_input_buffer[shell_input_cursor],
                &shell_input_buffer[shell_input_cursor + 1U],
                trailing * sizeof(uint32_t));
    }

    shell_input_length--;
    shell_input_buffer[shell_input_length] = 0U;
    shell_preferred_column_valid = false;
}

/** @brief Move the logical cursor one codepoint to the left. */
static void shell_cursor_left() {
    if (shell_input_cursor > 0U) {
        shell_input_cursor--;
    }
    shell_preferred_column_valid = false;
}

/** @brief Move the logical cursor one codepoint to the right. */
static void shell_cursor_right() {
    if (shell_input_cursor < shell_input_length) {
        shell_input_cursor++;
    }
    shell_preferred_column_valid = false;
}

/** @brief Move the logical cursor to the start of the input buffer. */
static void shell_cursor_home() {
    shell_input_cursor = 0U;
    shell_preferred_column_valid = false;
}

/** @brief Move the logical cursor to the end of the input buffer. */
static void shell_cursor_end() {
    shell_input_cursor = shell_input_length;
    shell_preferred_column_valid = false;
}

/**
 * @brief Move the cursor vertically while retaining its preferred visual column.
 *
 * @param[in] direction -1 for the previous row or 1 for the next row.
 */
static void shell_cursor_vertical(int direction) {
    size_t columns = shell_screen.columns;
    if (columns == 0U || (direction != -1 && direction != 1)) {
        return;
    }

    size_t absolute_index = shell_input_start_index + shell_input_cursor;
    size_t current_row = absolute_index / columns;
    size_t current_column = absolute_index % columns;

    if (!shell_preferred_column_valid) {
        shell_preferred_column = current_column;
        shell_preferred_column_valid = true;
    }

    if (direction < 0) {
        if (current_row == 0U) {
            return;
        }
        current_row--;
    } else {
        current_row++;
    }

    size_t target = current_row * columns + shell_preferred_column;
    if (target <= shell_input_start_index) {
        shell_input_cursor = 0U;
    } else if (target >= shell_input_start_index + shell_input_length) {
        shell_input_cursor = shell_input_length;
    } else {
        shell_input_cursor = target - shell_input_start_index;
    }
}

/**
 * @brief Commit the current input and append a fresh prompt after its final visual row.
 *
 * @return E_OK on success or an error code when the next prompt cannot be placed.
 */
static kerr_t shell_newline() {
    shell_input_cursor = shell_input_length;

    cursor_index = 0U;
    K_TRY(shell_rebuild_screen(&cursor_index));

    size_t columns = shell_screen.columns;
    size_t cell_count = columns * shell_screen.rows;
    size_t end_index = shell_input_start_index + shell_input_length;
    size_t end_row = end_index / columns;
    size_t view_start_row = (end_row >= shell_screen.rows)
                                ? end_row - shell_screen.rows + 1U
                                : 0U;
    size_t view_start_cell = view_start_row * columns;

    memcpy(shell_history_cells, shell_screen.cells,
           cell_count * sizeof(uint32_t));

    size_t next_prompt = end_index - view_start_cell;
    size_t column = next_prompt % columns;
    if (column != 0U) {
        next_prompt += columns - column;
    }

    while (next_prompt + (sizeof(shell_prompt) - 1U) >= cell_count) {
        shell_history_scroll(1U);
        if (next_prompt < columns) {
            return E_BADSIZE;
        }
        next_prompt -= columns;
    }

    K_TRY(shell_history_write_prompt(next_prompt));

    memset32(shell_input_buffer, 0U, SHELL_INPUT_MAX_CODEPOINTS + 1U);
    shell_input_length = 0U;
    shell_input_cursor = 0U;
    shell_preferred_column = 0U;
    shell_preferred_column_valid = false;
    return E_OK;
}

/**
 * @brief Render a copied shell screen and cursor position.
 *
 * @param[in] cursor_index Cursor position in the copied model.
 * @return E_OK on success or the renderer error.
 */
static kerr_t shell_render_snapshot(size_t cursor_index) {
    return shell_renderer_render(&shell_render_screen, cursor_index);
}

/**
 * @brief Render dirty shell snapshots without blocking the input dispatcher.
 *
 * @param[in] arg Unused thread argument.
 */
static void shell_renderer_thread(void *arg) {
    (void)arg;

    for (;;) {
        size_t cursor_index = 0U;
        bool running;
        bool dirty;

        flags = spinlock_acquire_irqsave(&shell_lock);
        running = shell_initialized;
        dirty = shell_dirty;
        if (dirty) {
            size_t total_cells = shell_screen.columns * shell_screen.rows;
            memcpy(shell_render_screen.cells, shell_screen.cells,
                   total_cells * sizeof(uint32_t));

            size_t absolute_cursor = shell_input_start_index + shell_input_cursor;
            size_t cursor_row = absolute_cursor / shell_screen.columns;
            size_t view_start_row = (cursor_row >= shell_screen.rows)
                                        ? cursor_row - shell_screen.rows + 1U
                                        : 0U;
            cursor_index = absolute_cursor -
                           (view_start_row * shell_screen.columns);
            shell_dirty = false;
        }
        spinlock_release_irqrestore(&shell_lock, flags);

        if (!running) {
            thread_exit();
        }

        if (dirty) {
            kerr_t err = shell_render_snapshot(cursor_index);
            if (k_failed(err)) {
                log_warn("shell render failed (%s)", kerr_str(err));
            }
        }

        thread_sleep_ms(SHELL_RENDER_INTERVAL_MS);
    }
}

/**
 * @brief Process translated keyboard events for the interactive shell.
 *
 * @param[in] event Translated input event from the input dispatcher.
 * @param[in] context Unused callback context.
 */
static void shell_input_handler(const input_event_t *event, void *context) {
    (void)context;

    if (!event || event->hid.type != HID_EVENT_KEY_PRESS ||
        (event->key_action != INPUT_KEY_ACTION_PRESSED &&
         event->key_action != INPUT_KEY_ACTION_REPEATED)) {
        return;
    }

    hid_keycode_t key = event->hid.key.keycode;
    if ((size_t)key >= INPUT_KEY_COUNT) {
        return;
    }

    if (key == KEY_RETURN &&
        event->key_action != INPUT_KEY_ACTION_PRESSED) {
        return;
    }

    if (event->unicode_count == 0U &&
        key != KEY_RETURN && key != KEY_BACK && key != KEY_DELETE &&
        key != KEY_LEFT && key != KEY_RIGHT && key != KEY_UP &&
        key != KEY_DOWN && key != KEY_HOME && key != KEY_END) {
        return;
    }

    keyboard_state_t keyboard;
    if (k_failed(keyboard_get_state(&keyboard)) ||
        keyboard.control || keyboard.alt) {
        return;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_lock);
    if (!shell_initialized) {
        spinlock_release_irqrestore(&shell_lock, flags);
        return;
    }

    kerr_t input_err = E_OK;

    if (key == KEY_RETURN) {
        input_err = shell_newline();
    } else if (key == KEY_BACK) {
        shell_backspace();
    } else if (key == KEY_DELETE) {
        shell_delete();
    } else if (key == KEY_LEFT) {
        shell_cursor_left();
    } else if (key == KEY_RIGHT) {
        shell_cursor_right();
    } else if (key == KEY_UP) {
        shell_cursor_vertical(-1);
    } else if (key == KEY_DOWN) {
        shell_cursor_vertical(1);
    } else if (key == KEY_HOME) {
        shell_cursor_home();
    } else if (key == KEY_END) {
        shell_cursor_end();
    } else {
        for (uint8_t i = 0U; i < event->unicode_count; i++) {
            uint32_t codepoint = (i == 0U)
                                     ? event->unicode
                                     : event->unicode_secondary;
            if (codepoint == (uint32_t)'\t') {
                shell_insert_tab();
            } else {
                shell_insert_codepoint(codepoint);
            }
        }
    }

    size_t cursor_index = 0U;
    kerr_t render_err = shell_rebuild_screen(&cursor_index);
    if (!k_failed(render_err)) {
        shell_dirty = true;
    } else if (!k_failed(input_err)) {
        input_err = render_err;
    }

    spinlock_release_irqrestore(&shell_lock, flags);

    if (k_failed(input_err)) {
        log_warn("shell input update failed (%s)", kerr_str(input_err));
    }
}

/**
 * @brief Initialize and display the interactive kernel shell.
 *
 * @return E_OK on success or an error code.
 */
kerr_t shell_init() {
    rect_size_t size = {0};
    size_t columns = 0U;
    size_t rows = 0U;
    size_t cell_count = 0U;
    virt_addr_t memory = 0U;
    kerr_t err = E_OK;
    size_t prompt_start = 0U;
    size_t cursor_index = 0U;
    irq_flags_t flags = 0U;
    thread_t *render_thread = NULL;

    if (shell_initialized) {
        return E_OK;
    }

    size = compositor_get_size();
    if (size.width < 8U || size.height < 16U) {
        return E_NOTAVAIL;
    }

    columns = (size_t)(size.width / 8U);
    rows = (size_t)(size.height / 16U);
    if (rows <= SHELL_PROMPT_ROW ||
        columns > SIZE_MAX / rows ||
        columns * rows > SIZE_MAX / sizeof(uint32_t) - 1U) {
        return E_NOTAVAIL;
    }

    cell_count = columns * rows;
    if (cell_count <= SHELL_PROMPT_ROW * columns +
                          (sizeof(shell_prompt) - 1U) + 1U) {
        return E_NOTAVAIL;
    }

    K_TRY(shell_screen_init(&shell_screen, columns, rows));
    err = shell_screen_init(&shell_render_screen, columns, rows);
    if (k_failed(err)) {
        shell_screen_destroy(&shell_screen);
        return err;
    }

    err = kzalloc(cell_count * sizeof(uint32_t), &memory);
    if (k_failed(err)) {
        goto fail_screens;
    }
    shell_history_cells = (uint32_t *)memory;

    memory = 0U;
    err = kzalloc((SHELL_INPUT_MAX_CODEPOINTS + 1U) * sizeof(uint32_t),
                  &memory);
    if (k_failed(err)) {
        goto fail_history;
    }
    shell_input_buffer = (uint32_t *)memory;

    memset32(shell_history_cells, 0U, cell_count);
    err = shell_screen_write(&shell_screen, 0U, SHELL_HEADER_ROW,
                             "NanoOS Kernel Shell");
    if (!k_failed(err)) {
        err = shell_screen_write(&shell_screen, 0U, SHELL_INFO_ROW,
                                 "Interactive input - command execution disabled");
    }
    if (!k_failed(err)) {
        err = shell_screen_write(&shell_screen, 0U, SHELL_STATUS_ROW,
                                 "Keyboard layout: de-DE");
    }
    if (k_failed(err)) {
        goto fail_input;
    }

    memcpy(shell_history_cells, shell_screen.cells,
           cell_count * sizeof(uint32_t));

    prompt_start = SHELL_PROMPT_ROW * columns;
    err = shell_history_write_prompt(prompt_start);
    if (k_failed(err)) {
        goto fail_input;
    }

    shell_input_length = 0U;
    shell_input_cursor = 0U;
    shell_preferred_column = 0U;
    shell_preferred_column_valid = false;

    cursor_index = 0U;
    err = shell_rebuild_screen(&cursor_index);
    if (k_failed(err)) {
        goto fail_input;
    }

    err = shell_renderer_init();
    if (k_failed(err)) {
        goto fail_input;
    }

    memcpy(shell_render_screen.cells, shell_screen.cells,
           cell_count * sizeof(uint32_t));
    err = shell_render_snapshot(cursor_index);
    if (k_failed(err)) {
        shell_renderer_deinit();
        goto fail_input;
    }

    flags = spinlock_acquire_irqsave(&shell_lock);
    shell_initialized = true;
    shell_dirty = false;
    spinlock_release_irqrestore(&shell_lock, flags);

    err = thread_create(proc_get_kernel_proc(), shell_renderer_thread,
                        NULL, "shell_renderer", &render_thread);
    if (k_failed(err)) {
        flags = spinlock_acquire_irqsave(&shell_lock);
        shell_initialized = false;
        spinlock_release_irqrestore(&shell_lock, flags);
        shell_renderer_deinit();
        goto fail_input;
    }

    err = input_register_handler(shell_input_handler, NULL);
    if (k_failed(err)) {
        flags = spinlock_acquire_irqsave(&shell_lock);
        shell_initialized = false;
        spinlock_release_irqrestore(&shell_lock, flags);
        shell_renderer_deinit();
        goto fail_input;
    }

    log_info("interactive shell input handler registered");
    return E_OK;

fail_input:
    if (shell_input_buffer) {
        (void)kfree((virt_addr_t)shell_input_buffer);
        shell_input_buffer = NULL;
    }
fail_history:
    if (shell_history_cells) {
        (void)kfree((virt_addr_t)shell_history_cells);
        shell_history_cells = NULL;
    }
fail_screens:
    (void)shell_screen_destroy(&shell_render_screen);
    (void)shell_screen_destroy(&shell_screen);
    shell_input_length = 0U;
    shell_input_cursor = 0U;
    shell_input_start_index = 0U;
    return err;
}
