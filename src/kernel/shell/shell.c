/**
 * @file shell.c
 * @brief Interactive character input for the kernel shell.
 * @author friedrichOsDev
 */

#include <drivers/gfx/compositor.h>
#include <drivers/hid/input.h>
#include <lib/util/mem.h>
#include <shell/renderer.h>
#include <shell/screen.h>
#include <shell/shell.h>

#define SHELL_HEADER_ROW 0U
#define SHELL_INFO_ROW 1U
#define SHELL_STATUS_ROW 2U
#define SHELL_PROMPT_ROW 4U

static const char shell_prompt[] = "nanoos> ";

static shell_screen_t shell_screen;
static size_t shell_cursor_index;
static size_t shell_input_start_index;
static bool shell_initialized;

/**
 * @brief Scroll the shell screen up by one character row.
 *
 * The cursor and input boundary are moved with the contents. If the beginning
 * of the current input line scrolls away, backspace is restricted to the
 * remaining visible input.
 */
static void shell_scroll(void) {
    size_t columns = shell_screen.columns;
    size_t total_cells = columns * shell_screen.rows;

    if (columns == 0U || shell_screen.rows == 0U) {
        return;
    }

    memmove(shell_screen.cells,
            shell_screen.cells + columns,
            (total_cells - columns) * sizeof(uint32_t));
    memset32(shell_screen.cells + (total_cells - columns), 0U, columns);

    if (shell_cursor_index >= columns) {
        shell_cursor_index -= columns;
    } else {
        shell_cursor_index = 0U;
    }

    if (shell_input_start_index >= columns) {
        shell_input_start_index -= columns;
    } else {
        shell_input_start_index = 0U;
    }
}

/**
 * @brief Keep the cursor inside the allocated screen model.
 */
static void shell_cursor_normalize(void) {
    size_t total_cells = shell_screen.columns * shell_screen.rows;

    while (shell_cursor_index >= total_cells) {
        shell_scroll();
    }
}

/**
 * @brief Render the screen and temporarily draw an underscore cursor.
 *
 * The cursor glyph is only a rendering overlay; it is not stored in the
 * terminal contents and therefore cannot corrupt typed text.
 *
 * @return E_OK on success or the renderer error.
 */
static kerr_t shell_render(void) {
    shell_cursor_normalize();

    uint32_t *cursor_cell = &shell_screen.cells[shell_cursor_index];
    uint32_t previous_cell = *cursor_cell;
    *cursor_cell = (uint32_t)'_';

    kerr_t err = shell_renderer_render(&shell_screen);
    *cursor_cell = previous_cell;
    return err;
}

/**
 * @brief Write the shell prompt at the current cursor position.
 *
 * @return E_OK on success or an error code.
 */
static kerr_t shell_write_prompt(void) {
    shell_cursor_normalize();

    size_t columns = shell_screen.columns;
    size_t row = shell_cursor_index / columns;
    size_t column = shell_cursor_index % columns;

    K_TRY(shell_screen_write(&shell_screen, column, row, shell_prompt));

    shell_cursor_index += sizeof(shell_prompt) - 1U;
    shell_cursor_normalize();
    shell_input_start_index = shell_cursor_index;
    return E_OK;
}

/**
 * @brief Insert one printable Unicode codepoint at the current cursor.
 *
 * @param[in] codepoint Unicode scalar value to insert.
 */
static void shell_insert_codepoint(uint32_t codepoint) {
    if (codepoint < 0x20U || codepoint == 0x7FU ||
        codepoint > 0x10FFFFU ||
        (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
        return;
    }

    shell_cursor_normalize();
    shell_screen.cells[shell_cursor_index] = codepoint;
    shell_cursor_index++;
    shell_cursor_normalize();
}

/**
 * @brief Insert a translated tab as four spaces.
 */
static void shell_insert_tab(void) {
    for (size_t i = 0U; i < 4U; i++) {
        shell_insert_codepoint((uint32_t)' ');
    }
}

/**
 * @brief Remove the character immediately before the cursor.
 */
static void shell_backspace(void) {
    if (shell_cursor_index <= shell_input_start_index) {
        return;
    }

    shell_cursor_index--;
    shell_screen.cells[shell_cursor_index] = 0U;
}

/**
 * @brief Finish the current input line and display another prompt.
 *
 * This stage deliberately does not parse or execute the entered text.
 *
 * @return E_OK on success or an error code.
 */
static kerr_t shell_newline(void) {
    size_t column = shell_cursor_index % shell_screen.columns;
    if (column != 0U) {
        shell_cursor_index += shell_screen.columns - column;
    }

    shell_cursor_normalize();
    return shell_write_prompt();
}

/**
 * @brief Process translated keyboard events for the interactive shell.
 *
 * @param[in] event Translated input event from the input dispatcher.
 * @param[in] context Unused callback context.
 */
static void shell_input_handler(const input_event_t *event, void *context) {
    (void)context;

    if (!shell_initialized || !event ||
        (event->hid.type != HID_EVENT_KEY_PRESS) ||
        (event->key_action != INPUT_KEY_ACTION_PRESSED &&
         event->key_action != INPUT_KEY_ACTION_REPEATED)) {
        return;
    }

    hid_keycode_t key = event->hid.key.keycode;
    if (key == KEY_RETURN) {
        if (event->key_action == INPUT_KEY_ACTION_PRESSED) {
            kerr_t err = shell_newline();
            if (k_failed(err)) {
                log_warn("shell newline failed (%s)", kerr_str(err));
            }
        }
    } else if (key == KEY_BACK) {
        shell_backspace();
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

    kerr_t err = shell_render();
    if (k_failed(err)) {
        log_warn("shell render failed after input (%s)", kerr_str(err));
    }
}

/**
 * @brief Initialize and display the interactive kernel shell.
 *
 * The shell registers itself with the central input dispatcher only after
 * its screen model and initial frame are fully initialized.
 *
 * @return E_OK on success or an error code.
 */
kerr_t shell_init(void) {
    if (shell_initialized) {
        return E_OK;
    }

    rect_size_t size = compositor_get_size();
    if (size.width < 8U || size.height < 16U) {
        return E_NOTAVAIL;
    }

    size_t columns = (size_t)(size.width / 8U);
    size_t rows = (size_t)(size.height / 16U);
    if (rows <= SHELL_PROMPT_ROW) {
        return E_NOTAVAIL;
    }

    K_TRY(shell_screen_init(&shell_screen, columns, rows));
    kerr_t err = shell_screen_clear(&shell_screen);
    if (k_failed(err)) {
        shell_screen_destroy(&shell_screen);
        return err;
    }

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
        shell_screen_destroy(&shell_screen);
        return err;
    }

    shell_cursor_index = SHELL_PROMPT_ROW * columns;
    err = shell_write_prompt();
    if (k_failed(err)) {
        shell_screen_destroy(&shell_screen);
        return err;
    }

    err = shell_renderer_init();
    if (k_failed(err)) {
        shell_screen_destroy(&shell_screen);
        return err;
    }

    err = shell_render();
    if (k_failed(err)) {
        shell_renderer_deinit();
        shell_screen_destroy(&shell_screen);
        return err;
    }

    shell_initialized = true;
    err = input_register_handler(shell_input_handler, NULL);
    if (k_failed(err)) {
        shell_initialized = false;
        shell_renderer_deinit();
        shell_screen_destroy(&shell_screen);
        return err;
    }

    log_info("interactive shell input handler registered");
    return E_OK;
}
