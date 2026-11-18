/**
 * @file renderer.c
 * @brief Shell input renderer.
 * @author friedrichOsDev
 */

#include "_shell.h"
#include <drivers/gfx/compositor.h>
#include <shell/output.h>
#include <kernel/sync.h>
#include <mm/memory.h>
#include <sched/sched.h>
#include <sched/thread.h>

#define SHELL_FONT_SCALE 1U
#define SHELL_RENDER_INTERVAL_MS 16U
#define SHELL_PROMPT "friedrichOsDev@nanoOS> "
#define SHELL_PROMPT_LENGTH 23U
#define SHELL_INPUT_TEXT_CAPACITY (SHELL_INPUT_BUFFER_SIZE * 4U + 1U)
#define SHELL_OUTPUT_TEXT_CAPACITY (SHELL_OUTPUT_MAX_LINE_LENGTH * 4U + 1U)

static shell_screen_t shell_screen;
static layer_t *shell_layer = NULL;
static spinlock_t shell_render_lock = SPINLOCK_INIT;
static shell_input_buffer_t shell_input_snapshot;
static volatile bool shell_renderer_initialized = false;
static volatile bool shell_renderer_stop_requested = false;
static volatile bool shell_renderer_stopped = true;
static char shell_input_text[SHELL_INPUT_TEXT_CAPACITY];
static char shell_output_text[SHELL_OUTPUT_TEXT_CAPACITY];
static shell_output_line_t *shell_output_snapshot_buffer = NULL;
static size_t shell_output_snapshot_capacity = 0U;
static size_t shell_output_snapshot_count = 0U;
static uint64_t shell_output_snapshot_generation = 0U;

/**
 * @brief Encodes one Unicode codepoint as UTF-8.
 *
 * @param[in] codepoint Unicode codepoint to encode.
 * @param[out] output Destination buffer with at least four bytes.
 * @return Number of bytes written.
 */
static size_t shell_encode_utf8(uint32_t codepoint, char *output) {
    if (codepoint > UINT32_C(0x10FFFF) ||
        (codepoint >= UINT32_C(0xD800) && codepoint <= UINT32_C(0xDFFF))) {
        codepoint = UINT32_C(0xFFFD);
    }
    if (codepoint <= 0x7FU) {
        output[0] = (char)codepoint;
        return 1U;
    }
    if (codepoint <= 0x7FFU) {
        output[0] = (char)(0xC0U | (codepoint >> 6));
        output[1] = (char)(0x80U | (codepoint & 0x3FU));
        return 2U;
    }
    if (codepoint <= 0xFFFFU) {
        output[0] = (char)(0xE0U | (codepoint >> 12));
        output[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        output[2] = (char)(0x80U | (codepoint & 0x3FU));
        return 3U;
    }
    output[0] = (char)(0xF0U | (codepoint >> 18));
    output[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
    output[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
    output[3] = (char)(0x80U | (codepoint & 0x3FU));
    return 4U;
}

/**
 * @brief Releases the dynamically allocated virtual screen buffers.
 *
 * @param[in,out] screen Screen whose buffers should be released.
 */
static void shell_screen_release(shell_screen_t *screen) {
    if (!screen) {
        return;
    }
    if (screen->buffer) {
        kfree((virt_addr_t)screen->buffer);
        screen->buffer = NULL;
    }
    if (screen->dirty_lines) {
        kfree((virt_addr_t)screen->dirty_lines);
        screen->dirty_lines = NULL;
    }
}

/**
 * @brief Allocates a screen grid matching the current compositor dimensions.
 *
 * @param[in] width Screen width in pixels.
 * @param[in] height Screen height in pixels.
 * @return E_OK on success, or an error code.
 */
static kerr_t shell_screen_configure(uint64_t width, uint64_t height) {
    if (width == 0U || height == 0U) {
        return E_NOTAVAIL;
    }

    rect_size_t glyph_size = {0};
    K_TRY(compositor_get_font_size(&glyph_size));
    if (glyph_size.width == 0U || glyph_size.height == 0U) {
        return E_NOTAVAIL;
    }

    uint64_t scaled_glyph_width = glyph_size.width * SHELL_FONT_SCALE;
    uint64_t scaled_glyph_height = glyph_size.height * SHELL_FONT_SCALE;
    uint64_t columns = width / scaled_glyph_width;
    uint64_t lines = height / scaled_glyph_height;
    if (columns == 0U || lines == 0U || columns > SIZE_MAX / lines) {
        return E_NOTAVAIL;
    }

    size_t cell_count = (size_t)(columns * lines);
    if (cell_count > SIZE_MAX / sizeof(shell_cell_t) ||
        lines > SIZE_MAX / sizeof(bool)) {
        return E_NOTAVAIL;
    }

    virt_addr_t buffer_mem = 0;
    K_TRY(kzalloc(cell_count * sizeof(shell_cell_t), &buffer_mem));

    virt_addr_t dirty_mem = 0;
    kerr_t err = kzalloc((size_t)lines * sizeof(bool), &dirty_mem);
    if (k_failed(err)) {
        kfree(buffer_mem);
        return err;
    }

    size_t output_capacity = (lines > 1U) ? (size_t)(lines - 1U) : 0U;
    virt_addr_t output_mem = 0U;
    if (output_capacity > 0U) {
        if (output_capacity > SIZE_MAX / sizeof(shell_output_line_t)) {
            kfree(buffer_mem);
            kfree(dirty_mem);
            return E_NOTAVAIL;
        }
        err = kzalloc(output_capacity * sizeof(shell_output_line_t), &output_mem);
        if (k_failed(err)) {
            kfree(buffer_mem);
            kfree(dirty_mem);
            return err;
        }
    }

    shell_screen_t new_screen = {
        .font_scale = SHELL_FONT_SCALE,
        .lines = lines,
        .columns = columns,
        .buffer_size = cell_count,
        .buffer = (shell_cell_t *)buffer_mem,
        .is_dirty = true,
        .dirty_lines = (bool *)dirty_mem,
        .pixel_width = width,
        .pixel_height = height,
        .glyph_width = glyph_size.width,
        .glyph_height = glyph_size.height,
        .input_scroll = 0U,
        .input_visible_count = 0U,
        .prompt_columns = 0U,
        .cursor_column = 0U,
        .cursor_line = lines - 1U,
        .cursor_visible = false};

    for (uint64_t line = 0U; line < lines; line++) {
        new_screen.dirty_lines[line] = false;
    }
    new_screen.dirty_lines[lines - 1U] = true;

    if (shell_layer) {
        err = layer_set_size(shell_layer, (rect_size_t){width, height});
    } else {
        err = layer_create((rect_size_t){width, height}, 100, &shell_layer);
        if (!k_failed(err)) {
            err = layer_set_position(shell_layer, (point_t){0, 0});
        }
    }

    if (k_failed(err)) {
        if (shell_layer && shell_screen.buffer == NULL) {
            layer_destroy(shell_layer);
            shell_layer = NULL;
        }
        shell_screen_release(&new_screen);
        if (output_mem != 0U) {
            kfree(output_mem);
        }
        return err;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_render_lock);
    shell_screen_t old_screen = shell_screen;
    shell_screen = new_screen;
    spinlock_release_irqrestore(&shell_render_lock, flags);
    shell_screen_release(&old_screen);

    shell_output_line_t *old_output_snapshot = shell_output_snapshot_buffer;
    shell_output_snapshot_buffer = (shell_output_line_t *)output_mem;
    shell_output_snapshot_capacity = output_capacity;
    shell_output_snapshot_count = 0U;
    shell_output_snapshot_generation = 0U;
    if (old_output_snapshot) {
        kfree((virt_addr_t)old_output_snapshot);
    }
    return E_OK;
}

/** @brief Composes the visible prompt and input into the bottom screen row. */
static void compose_frame() {
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_render_lock);
    if (!shell_screen.buffer || !shell_screen.dirty_lines ||
        shell_screen.lines == 0U || shell_screen.columns == 0U) {
        spinlock_release_irqrestore(&shell_render_lock, flags);
        return;
    }

    uint64_t row = shell_screen.lines - 1U;
    uint64_t row_offset = row * shell_screen.columns;
    uint64_t prompt_columns = SHELL_PROMPT_LENGTH;
    if (prompt_columns > shell_screen.columns) {
        prompt_columns = shell_screen.columns;
    }

    uint64_t input_columns = shell_screen.columns - prompt_columns;
    uint64_t input_size = shell_input_snapshot.size;
    uint64_t cursor_pos = shell_input_snapshot.cursor_pos;
    if (input_size > SHELL_INPUT_BUFFER_SIZE) {
        input_size = SHELL_INPUT_BUFFER_SIZE;
    }
    if (cursor_pos > input_size) {
        cursor_pos = input_size;
    }

    uint64_t scroll = 0U;
    if (input_columns > 0U) {
        if (cursor_pos >= input_columns) {
            scroll = cursor_pos - input_columns + 1U;
        }
        uint64_t max_scroll = (input_size + 1U > input_columns)
                                  ? input_size + 1U - input_columns
                                  : 0U;
        if (scroll > max_scroll) {
            scroll = max_scroll;
        }
    }

    for (uint64_t column = 0U; column < shell_screen.columns; column++) {
        shell_cell_t *cell = &shell_screen.buffer[row_offset + column];
        cell->unicode = (uint32_t)' ';
        cell->fg = COLOR_WHITE;
        cell->bg = COLOR_TRANSPARENT;
    }
    for (uint64_t column = 0U; column < prompt_columns; column++) {
        shell_screen.buffer[row_offset + column].unicode = (uint32_t)SHELL_PROMPT[column];
        shell_screen.buffer[row_offset + column].fg = COLOR_LIGHT_GRAY;
    }

    uint64_t visible_count = 0U;
    if (input_columns > 0U && scroll < input_size) {
        visible_count = input_size - scroll;
        if (visible_count > input_columns) {
            visible_count = input_columns;
        }
        for (uint64_t i = 0U; i < visible_count; i++) {
            shell_screen.buffer[row_offset + prompt_columns + i].unicode =
                shell_input_snapshot.buffer[scroll + i];
        }
    }

    shell_screen.input_scroll = scroll;
    shell_screen.input_visible_count = visible_count;
    shell_screen.prompt_columns = prompt_columns;
    shell_screen.cursor_line = row;
    shell_screen.cursor_visible = false;
    if (input_columns > 0U && cursor_pos >= scroll &&
        cursor_pos - scroll < input_columns) {
        shell_screen.cursor_column = prompt_columns + cursor_pos - scroll;
        shell_screen.cursor_visible = true;
    }

    shell_screen.dirty_lines[row] = true;
    shell_screen.is_dirty = true;
    spinlock_release_irqrestore(&shell_render_lock, flags);
}

/** @brief Renders the composed bottom input row into the shell compositor layer. */
static void render_frame() {
    char prompt_text[SHELL_PROMPT_LENGTH + 1U];
    uint64_t input_length = 0U;
    uint64_t prompt_columns = 0U;
    uint64_t cursor_column = 0U;
    uint64_t glyph_width = 0U;
    uint64_t glyph_height = 0U;
    uint64_t scale = 1U;
    uint64_t pixel_width = 0U;
    uint64_t pixel_height = 0U;
    bool cursor_visible = false;
    bool cursor_has_character = false;
    char cursor_text[5] = {0};

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_render_lock);
    if (!shell_screen.is_dirty || !shell_screen.buffer ||
        !shell_screen.dirty_lines || !shell_layer) {
        spinlock_release_irqrestore(&shell_render_lock, flags);
        return;
    }

    uint64_t row = shell_screen.cursor_line;
    uint64_t row_offset = row * shell_screen.columns;
    prompt_columns = shell_screen.prompt_columns;
    if (prompt_columns > SHELL_PROMPT_LENGTH) {
        prompt_columns = SHELL_PROMPT_LENGTH;
    }
    for (uint64_t i = 0U; i < prompt_columns; i++) {
        prompt_text[i] = SHELL_PROMPT[i];
    }
    prompt_text[prompt_columns] = '\0';

    for (uint64_t i = 0U; i < shell_screen.input_visible_count; i++) {
        uint32_t codepoint = shell_screen.buffer[row_offset + prompt_columns + i].unicode;
        input_length += shell_encode_utf8(codepoint, &shell_input_text[input_length]);
    }
    shell_input_text[input_length] = '\0';

    cursor_column = shell_screen.cursor_column;
    cursor_visible = shell_screen.cursor_visible;
    if (cursor_visible && cursor_column >= prompt_columns) {
        uint64_t input_column = cursor_column - prompt_columns;
        if (input_column < shell_screen.input_visible_count) {
            uint32_t cursor_codepoint =
                shell_screen.buffer[row_offset + cursor_column].unicode;
            (void)shell_encode_utf8(cursor_codepoint, cursor_text);
            cursor_has_character = true;
        }
    }
    glyph_width = shell_screen.glyph_width;
    glyph_height = shell_screen.glyph_height;
    scale = shell_screen.font_scale;
    pixel_width = shell_screen.pixel_width;
    pixel_height = shell_screen.pixel_height;

    shell_screen.dirty_lines[row] = false;
    shell_screen.is_dirty = false;
    spinlock_release_irqrestore(&shell_render_lock, flags);

    if (k_failed(layer_draw_begin(shell_layer))) {
        flags = spinlock_acquire_irqsave(&shell_render_lock);
        shell_screen.is_dirty = true;
        if (row < shell_screen.lines) {
            shell_screen.dirty_lines[row] = true;
        }
        spinlock_release_irqrestore(&shell_render_lock, flags);
        return;
    }

    uint64_t scaled_width = glyph_width * scale;
    uint64_t scaled_height = glyph_height * scale;
    uint64_t line_y = pixel_height - scaled_height;

    canvas_clear(&shell_layer->canvas, COLOR_TRANSPARENT);

    /*
     * Output is bottom-aligned in the area above the fixed input row.
     * The snapshot already contains only the newest lines that fit.
     */
    uint64_t output_rows = (pixel_height / scaled_height);
    if (output_rows > 0U) {
        output_rows--;
    }
    size_t visible_output_count = shell_output_snapshot_count;
    if (visible_output_count > output_rows) {
        visible_output_count = (size_t)output_rows;
    }
    size_t first_output_row = (size_t)output_rows - visible_output_count;

    for (size_t i = 0U; i < visible_output_count; i++) {
        const shell_output_line_t *line = &shell_output_snapshot_buffer[i];
        size_t text_length = 0U;

        for (uint16_t j = 0U; j < line->length; j++) {
            text_length += shell_encode_utf8(
                line->codepoints[j], &shell_output_text[text_length]);
        }
        shell_output_text[text_length] = '\0';

        if (text_length > 0U) {
            uint64_t output_y = ((uint64_t)first_output_row + i) * scaled_height;
            canvas_draw_text(
                &shell_layer->canvas,
                (point_t){0, (int64_t)output_y},
                shell_output_text,
                COLOR_LIGHT_GRAY,
                (uint32_t)scale);
        }
    }

    canvas_draw_rectangle(
        &shell_layer->canvas,
        (point_t){0, (int64_t)line_y},
        (rect_size_t){pixel_width, scaled_height},
        MAKE_COLOR_ALPHA(12, 16, 24, 232),
        true,
        0U);

    if (prompt_columns > 0U) {
        canvas_draw_text(
            &shell_layer->canvas,
            (point_t){0, (int64_t)line_y},
            prompt_text,
            COLOR_LIGHT_GRAY,
            (uint32_t)scale);
    }
    if (input_length > 0U) {
        canvas_draw_text(
            &shell_layer->canvas,
            (point_t){(int64_t)(prompt_columns * scaled_width), (int64_t)line_y},
            shell_input_text,
            COLOR_WHITE,
            (uint32_t)scale);
    }
    if (cursor_visible) {
        point_t cursor_position = {
            (int64_t)(cursor_column * scaled_width),
            (int64_t)line_y};

        /* Draw a solid cursor cell, then redraw its character with inverted colors. */
        canvas_draw_rectangle(
            &shell_layer->canvas,
            cursor_position,
            (rect_size_t){scaled_width, scaled_height},
            COLOR_WHITE,
            true,
            0U);

        if (cursor_has_character) {
            canvas_draw_text(
                &shell_layer->canvas,
                cursor_position,
                cursor_text,
                COLOR_BLACK,
                (uint32_t)scale);
        }
    }
    layer_draw_end(shell_layer);
}

/** @brief Report whether renderer-owned resources have been released. */
bool shell_renderer_is_stopped(void) {
    return __atomic_load_n(&shell_renderer_stopped, __ATOMIC_ACQUIRE);
}

/** @brief Releases renderer-owned resources after the render loop stops. */
static void shell_renderer_cleanup() {
    if (shell_layer) {
        (void)layer_destroy(shell_layer);
        shell_layer = NULL;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_render_lock);
    shell_screen_t old_screen = shell_screen;
    shell_screen = (shell_screen_t){0};
    spinlock_release_irqrestore(&shell_render_lock, flags);

    shell_screen_release(&old_screen);

    if (shell_output_snapshot_buffer) {
        kfree((virt_addr_t)shell_output_snapshot_buffer);
        shell_output_snapshot_buffer = NULL;
    }
    shell_output_snapshot_capacity = 0U;
    shell_output_snapshot_count = 0U;
    shell_output_snapshot_generation = 0U;

    __atomic_store_n(&shell_renderer_stopped, true, __ATOMIC_RELEASE);
}

/**
 * @brief Main shell renderer thread.
 *
 * @param[in] arg Unused thread argument.
 */
void shell_render_thread(void *arg) {
    (void)arg;

    while (!__atomic_load_n(&shell_renderer_stop_requested, __ATOMIC_ACQUIRE)) {
        rect_size_t screen_size = compositor_get_size();
        bool resized = false;
        if (screen_size.width != shell_screen.pixel_width ||
            screen_size.height != shell_screen.pixel_height) {
            if (!k_failed(shell_screen_configure(screen_size.width, screen_size.height))) {
                resized = true;
            }
        }

        bool input_changed = shell_sync_input_buffer(&shell_input_snapshot);
        bool output_changed = false;
        size_t output_count = 0U;
        kerr_t output_err = shell_output_snapshot(
            shell_output_snapshot_buffer,
            shell_output_snapshot_capacity,
            &output_count,
            &shell_output_snapshot_generation,
            &output_changed);

        if (!k_failed(output_err) && output_changed) {
            shell_output_snapshot_count = output_count;
        }

        if (resized || input_changed || output_changed) {
            compose_frame();
        }

        render_frame();
        thread_sleep_ms(SHELL_RENDER_INTERVAL_MS);
    }

    shell_renderer_cleanup();
    thread_exit();
}

/**
 * @brief Initializes the shell screen and starts its renderer thread.
 *
 * @return E_OK on success, or an error code.
 */
kerr_t shell_init_renderer() {
    if (__atomic_load_n(&shell_renderer_initialized, __ATOMIC_ACQUIRE)) {
        return E_OK;
    }
    if (!__atomic_load_n(&shell_renderer_stopped, __ATOMIC_ACQUIRE)) {
        return E_NOTAVAIL;
    }

    rect_size_t screen_size = compositor_get_size();
    K_TRY(shell_screen_configure(screen_size.width, screen_size.height));

    __atomic_store_n(&shell_renderer_stop_requested, false, __ATOMIC_RELEASE);
    __atomic_store_n(&shell_renderer_stopped, false, __ATOMIC_RELEASE);
    __atomic_store_n(&shell_renderer_initialized, true, __ATOMIC_RELEASE);

    thread_t *renderer_thread = NULL;
    kerr_t err = thread_create(NULL, shell_render_thread, NULL,
                               "shell_renderer", &renderer_thread);
    if (k_failed(err)) {
        __atomic_store_n(&shell_renderer_initialized, false, __ATOMIC_RELEASE);
        __atomic_store_n(&shell_renderer_stopped, true, __ATOMIC_RELEASE);
        if (shell_layer) {
            (void)layer_destroy(shell_layer);
            shell_layer = NULL;
        }
        shell_screen_release(&shell_screen);
        shell_screen = (shell_screen_t){0};
        if (shell_output_snapshot_buffer) {
            kfree((virt_addr_t)shell_output_snapshot_buffer);
            shell_output_snapshot_buffer = NULL;
        }
        shell_output_snapshot_capacity = 0U;
        shell_output_snapshot_count = 0U;
        shell_output_snapshot_generation = 0U;
        return err;
    }

    return E_OK;
}

/**
 * @brief Requests renderer shutdown without waiting for the renderer thread.
 *
 * The renderer thread releases its own layer and screen buffers. This function
 * must be called again after that cleanup has completed to finish deinit.
 *
 * @return E_OK when stopped, E_NOTAVAIL while shutdown is pending.
 */
kerr_t shell_deinit_renderer() {
    if (!__atomic_load_n(&shell_renderer_initialized, __ATOMIC_ACQUIRE)) {
        return __atomic_load_n(&shell_renderer_stopped, __ATOMIC_ACQUIRE)
                   ? E_OK
                   : E_NOTAVAIL;
    }

    __atomic_store_n(&shell_renderer_stop_requested, true, __ATOMIC_RELEASE);

    if (!__atomic_load_n(&shell_renderer_stopped, __ATOMIC_ACQUIRE)) {
        return E_NOTAVAIL;
    }

    __atomic_store_n(&shell_renderer_initialized, false, __ATOMIC_RELEASE);
    __atomic_store_n(&shell_renderer_stop_requested, false, __ATOMIC_RELEASE);
    return E_OK;
}
