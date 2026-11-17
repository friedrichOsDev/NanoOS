/**
 * @file output.c
 * @brief Thread-safe shell output buffer and UTF-8 writer.
 * @author friedrichOsDev
 */

#include <shell/output.h>
#include <kernel/sync.h>

#define SHELL_OUTPUT_TAB_WIDTH 4U

typedef struct {
    shell_output_line_t lines[SHELL_OUTPUT_MAX_LINES];
    size_t first_line;
    size_t line_count;
    uint64_t generation;
    bool initialized;
} shell_output_state_t;

static shell_output_state_t shell_output_state;
static spinlock_t shell_output_lock = SPINLOCK_INIT;

/* Keep the self-test snapshot out of the kernel thread stack. */
static shell_output_line_t shell_output_test_snapshot[4];

/**
 * @brief Returns the physical ring index for a logical line index.
 *
 * @param[in] logical_index Zero-based index from the oldest line.
 * @return Physical index in the ring.
 */
static size_t shell_output_ring_index(size_t logical_index) {
    return (shell_output_state.first_line + logical_index) % SHELL_OUTPUT_MAX_LINES;
}

/**
 * @brief Marks the output as changed.
 */
static void shell_output_mark_changed(void) {
    shell_output_state.generation++;
    if (shell_output_state.generation == 0U) {
        shell_output_state.generation = 1U;
    }
}

/**
 * @brief Appends a new empty line, dropping the oldest line when full.
 */
static void shell_output_push_line(void) {
    size_t next_index;

    if (shell_output_state.line_count < SHELL_OUTPUT_MAX_LINES) {
        next_index = shell_output_ring_index(shell_output_state.line_count);
        shell_output_state.line_count++;
    } else {
        shell_output_state.first_line =
            (shell_output_state.first_line + 1U) % SHELL_OUTPUT_MAX_LINES;
        next_index = shell_output_ring_index(shell_output_state.line_count - 1U);
    }

    shell_output_state.lines[next_index].length = 0U;
}

/**
 * @brief Appends one codepoint while the output lock is held.
 *
 * @param[in] codepoint Unicode codepoint to append.
 */
static void shell_output_append_locked(uint32_t codepoint) {
    shell_output_line_t *line;
    size_t line_index;

    if (codepoint == (uint32_t)'\n') {
        shell_output_push_line();
        shell_output_mark_changed();
        return;
    }

    if (codepoint == (uint32_t)'\r') {
        return;
    }

    if (codepoint == (uint32_t)'\t') {
        line_index = shell_output_ring_index(shell_output_state.line_count - 1U);
        line = &shell_output_state.lines[line_index];

        size_t spaces = SHELL_OUTPUT_TAB_WIDTH -
                        ((size_t)line->length % SHELL_OUTPUT_TAB_WIDTH);
        for (size_t i = 0U; i < spaces; i++) {
            if (line->length >= SHELL_OUTPUT_MAX_LINE_LENGTH) {
                shell_output_push_line();
                line_index = shell_output_ring_index(shell_output_state.line_count - 1U);
                line = &shell_output_state.lines[line_index];
            }
            line->codepoints[line->length++] = (uint32_t)' ';
        }
        shell_output_mark_changed();
        return;
    }

    if (codepoint > UINT32_C(0x10FFFF) ||
        (codepoint >= UINT32_C(0xD800) && codepoint <= UINT32_C(0xDFFF))) {
        codepoint = UINT32_C(0xFFFD);
    }

    line_index = shell_output_ring_index(shell_output_state.line_count - 1U);
    line = &shell_output_state.lines[line_index];

    if (line->length >= SHELL_OUTPUT_MAX_LINE_LENGTH) {
        shell_output_push_line();
        line_index = shell_output_ring_index(shell_output_state.line_count - 1U);
        line = &shell_output_state.lines[line_index];
    }

    line->codepoints[line->length++] = codepoint;
    shell_output_mark_changed();
}

/**
 * @brief Decodes one UTF-8 codepoint, replacing malformed sequences.
 *
 * Invalid leading or continuation bytes consume one byte so subsequent input
 * remains recoverable.
 *
 * @param[in] text Input byte array.
 * @param[in] length Total input length.
 * @param[in,out] offset Current byte offset; advanced by consumed bytes.
 * @return Decoded Unicode codepoint or U+FFFD.
 */
static uint32_t shell_output_decode_utf8(
    const uint8_t *text,
    size_t length,
    size_t *offset) {
    uint8_t first = text[*offset];
    uint32_t codepoint;
    size_t needed;

    if (first <= 0x7FU) {
        (*offset)++;
        return first;
    }

    if (first >= 0xC2U && first <= 0xDFU) {
        codepoint = first & 0x1FU;
        needed = 1U;
    } else if (first >= 0xE0U && first <= 0xEFU) {
        codepoint = first & 0x0FU;
        needed = 2U;
    } else if (first >= 0xF0U && first <= 0xF4U) {
        codepoint = first & 0x07U;
        needed = 3U;
    } else {
        (*offset)++;
        return UINT32_C(0xFFFD);
    }

    if (needed > length - *offset - 1U) {
        (*offset)++;
        return UINT32_C(0xFFFD);
    }

    for (size_t i = 1U; i <= needed; i++) {
        uint8_t continuation = text[*offset + i];
        if ((continuation & 0xC0U) != 0x80U) {
            (*offset)++;
            return UINT32_C(0xFFFD);
        }
        codepoint = (codepoint << 6) | (continuation & 0x3FU);
    }

    if ((needed == 1U && codepoint < 0x80U) ||
        (needed == 2U && codepoint < 0x800U) ||
        (needed == 3U && codepoint < 0x10000U) ||
        codepoint > UINT32_C(0x10FFFF) ||
        (codepoint >= UINT32_C(0xD800) && codepoint <= UINT32_C(0xDFFF))) {
        (*offset)++;
        return UINT32_C(0xFFFD);
    }

    *offset += needed + 1U;
    return codepoint;
}

/**
 * @brief Initializes the output ring with one empty line.
 *
 * @return E_OK on success.
 */
kerr_t shell_output_init(void) {
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_output_lock);

    shell_output_state.first_line = 0U;
    shell_output_state.line_count = 1U;
    for (size_t i = 0U; i < SHELL_OUTPUT_MAX_LINES; i++) {
        shell_output_state.lines[i].length = 0U;
    }
    shell_output_state.generation = 1U;
    shell_output_state.initialized = true;

    spinlock_release_irqrestore(&shell_output_lock, flags);
    return E_OK;
}

/**
 * @brief Disables and clears the output ring.
 *
 * @return E_OK on success.
 */
kerr_t shell_output_deinit(void) {
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_output_lock);

    shell_output_state.initialized = false;
    shell_output_state.first_line = 0U;
    shell_output_state.line_count = 0U;
    shell_output_state.generation++;

    spinlock_release_irqrestore(&shell_output_lock, flags);
    return E_OK;
}

/**
 * @brief Writes a null-terminated UTF-8 string.
 *
 * @param[in] text Null-terminated UTF-8 string.
 * @return E_OK on success, E_BADADDR for a null pointer, or E_NOINIT.
 */
kerr_t shell_write(const char *text) {
    if (!text) {
        return E_BADADDR;
    }

    size_t length = 0U;
    while (text[length] != '\0') {
        length++;
    }

    return shell_write_n(text, length);
}

/**
 * @brief Writes a byte range containing UTF-8 text.
 *
 * @param[in] text Input bytes.
 * @param[in] length Number of bytes.
 * @return E_OK on success, E_BADADDR for invalid input, or E_NOINIT.
 */
kerr_t shell_write_n(const char *text, size_t length) {
    if (!text && length != 0U) {
        return E_BADADDR;
    }
    if (length == 0U) {
        return E_OK;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_output_lock);
    if (!shell_output_state.initialized) {
        spinlock_release_irqrestore(&shell_output_lock, flags);
        return E_NOINIT;
    }

    size_t offset = 0U;
    while (offset < length) {
        uint32_t codepoint = shell_output_decode_utf8(
            (const uint8_t *)text, length, &offset);
        shell_output_append_locked(codepoint);
    }

    spinlock_release_irqrestore(&shell_output_lock, flags);
    return E_OK;
}

/**
 * @brief Writes one Unicode codepoint.
 *
 * @param[in] codepoint Unicode codepoint to append.
 * @return E_OK on success, or E_NOINIT.
 */
kerr_t shell_write_u32(uint32_t codepoint) {
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_output_lock);
    if (!shell_output_state.initialized) {
        spinlock_release_irqrestore(&shell_output_lock, flags);
        return E_NOINIT;
    }

    shell_output_append_locked(codepoint);
    spinlock_release_irqrestore(&shell_output_lock, flags);
    return E_OK;
}

/**
 * @brief Writes a string and appends a newline.
 *
 * @param[in] text Null-terminated UTF-8 string.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_write_line(const char *text) {
    K_TRY(shell_write(text));
    return shell_write_u32((uint32_t)'\n');
}

/**
 * @brief Clears the output and restores one empty current line.
 *
 * @return E_OK on success, or E_NOINIT.
 */
kerr_t shell_clear_output(void) {
    irq_flags_t flags = spinlock_acquire_irqsave(&shell_output_lock);
    if (!shell_output_state.initialized) {
        spinlock_release_irqrestore(&shell_output_lock, flags);
        return E_NOINIT;
    }

    shell_output_state.first_line = 0U;
    shell_output_state.line_count = 1U;
    shell_output_state.lines[0].length = 0U;
    shell_output_mark_changed();

    spinlock_release_irqrestore(&shell_output_lock, flags);
    return E_OK;
}

/**
 * @brief Copies the newest available output lines into a caller-owned snapshot.
 *
 * @param[out] lines Destination line array.
 * @param[in] capacity Number of destination entries.
 * @param[out] out_count Number of copied lines.
 * @param[in,out] inout_generation Previous and resulting generation.
 * @param[out] out_changed Whether a new snapshot was copied.
 * @return E_OK, E_BADADDR, or E_NOINIT.
 */
kerr_t shell_output_snapshot(
    shell_output_line_t *lines,
    size_t capacity,
    size_t *out_count,
    uint64_t *inout_generation,
    bool *out_changed) {
    if (!out_count || !inout_generation || !out_changed ||
        (capacity > 0U && !lines)) {
        return E_BADADDR;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&shell_output_lock);
    if (!shell_output_state.initialized) {
        spinlock_release_irqrestore(&shell_output_lock, flags);
        return E_NOINIT;
    }

    *out_count = 0U;
    *out_changed = (*inout_generation != shell_output_state.generation);
    if (!*out_changed) {
        spinlock_release_irqrestore(&shell_output_lock, flags);
        return E_OK;
    }

    size_t copy_count = shell_output_state.line_count;
    if (copy_count > capacity) {
        copy_count = capacity;
    }

    size_t first_to_copy = shell_output_state.line_count - copy_count;
    for (size_t i = 0U; i < copy_count; i++) {
        size_t source_index = shell_output_ring_index(first_to_copy + i);
        lines[i] = shell_output_state.lines[source_index];
    }

    *out_count = copy_count;
    *inout_generation = shell_output_state.generation;

    spinlock_release_irqrestore(&shell_output_lock, flags);
    return E_OK;
}

/**
 * @brief Runs a destructive API smoke test and leaves its output visible.
 *
 * @return E_OK when the expected lines and Unicode codepoint are present.
 */
kerr_t shell_output_self_test(void) {
    size_t count = 0U;
    uint64_t generation = 0U;
    bool changed = false;

    K_TRY(shell_clear_output());
    K_TRY(shell_write_line("Shell output API test"));
    K_TRY(shell_write("UTF-8: "));
    K_TRY(shell_write_u32(UINT32_C(0x00E4)));
    K_TRY(shell_write_line(" (ae)"));
    K_TRY(shell_write_line("Ring buffer and newline: OK"));

    K_TRY(shell_output_snapshot(
        shell_output_test_snapshot, 4U, &count, &generation, &changed));

    if (!changed || count != 4U) {
        return E_CHECKSUM;
    }
    if (shell_output_test_snapshot[0].length != 21U ||
        shell_output_test_snapshot[0].codepoints[0] != (uint32_t)'S' ||
        shell_output_test_snapshot[1].length < 8U ||
        shell_output_test_snapshot[1].codepoints[7] != UINT32_C(0x00E4)) {
        return E_CHECKSUM;
    }

    return E_OK;
}
