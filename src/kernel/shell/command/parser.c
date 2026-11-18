/**
 * @file parser.c
 * @brief Small, bounded shell command parser.
 * @author friedrichOsDev
 */

#include "parser.h"

/**
 * @brief Test the Unicode White_Space codepoint set used by the shell parser.
 *
 * This intentionally avoids a libc locale dependency.
 */
static bool shell_parser_is_whitespace(uint32_t codepoint) {
    if (codepoint >= UINT32_C(0x0009) && codepoint <= UINT32_C(0x000D)) {
        return true;
    }

    switch (codepoint) {
    case UINT32_C(0x0020):
    case UINT32_C(0x0085):
    case UINT32_C(0x00A0):
    case UINT32_C(0x1680):
    case UINT32_C(0x2028):
    case UINT32_C(0x2029):
    case UINT32_C(0x202F):
    case UINT32_C(0x205F):
    case UINT32_C(0x3000):
        return true;
    default:
        break;
    }

    return codepoint >= UINT32_C(0x2000) && codepoint <= UINT32_C(0x200A);
}

/**
 * @brief Append one Unicode scalar value as UTF-8 to parser storage.
 */
static kerr_t shell_parser_append_utf8(shell_parse_result_t *result, uint32_t codepoint) {
    uint8_t encoded[4];
    size_t encoded_length;

    if (codepoint == 0U ||
        codepoint > UINT32_C(0x10FFFF) ||
        (codepoint >= UINT32_C(0xD800) && codepoint <= UINT32_C(0xDFFF))) {
        return E_INVAL;
    }

    if (codepoint <= UINT32_C(0x7F)) {
        encoded[0] = (uint8_t)codepoint;
        encoded_length = 1U;
    } else if (codepoint <= UINT32_C(0x7FF)) {
        encoded[0] = (uint8_t)(0xC0U | (codepoint >> 6));
        encoded[1] = (uint8_t)(0x80U | (codepoint & 0x3FU));
        encoded_length = 2U;
    } else if (codepoint <= UINT32_C(0xFFFF)) {
        encoded[0] = (uint8_t)(0xE0U | (codepoint >> 12));
        encoded[1] = (uint8_t)(0x80U | ((codepoint >> 6) & 0x3FU));
        encoded[2] = (uint8_t)(0x80U | (codepoint & 0x3FU));
        encoded_length = 3U;
    } else {
        encoded[0] = (uint8_t)(0xF0U | (codepoint >> 18));
        encoded[1] = (uint8_t)(0x80U | ((codepoint >> 12) & 0x3FU));
        encoded[2] = (uint8_t)(0x80U | ((codepoint >> 6) & 0x3FU));
        encoded[3] = (uint8_t)(0x80U | (codepoint & 0x3FU));
        encoded_length = 4U;
    }

    if (result->storage_used > sizeof(result->storage) - encoded_length) {
        return E_BADSIZE;
    }

    for (size_t i = 0U; i < encoded_length; i++) {
        result->storage[result->storage_used++] = (char)encoded[i];
    }

    return E_OK;
}

/**
 * @brief Parse a command line according to the shell's deliberately small grammar.
 *
 * Outside quotes, Unicode whitespace separates arguments. Double quotes may
 * appear inside a token and concatenate with adjacent unquoted text. Within
 * quotes, only backslash-quote and backslash-backslash are escape sequences.
 */
kerr_t shell_parse(const uint32_t *input, size_t length, shell_parse_result_t *result) {
    if (!input || !result) {
        return E_BADADDR;
    }
    if (length > SHELL_COMMAND_MAX_LINE_CODEPOINTS) {
        return E_BADSIZE;
    }

    result->argc = 0U;
    result->storage_used = 0U;

    size_t position = 0U;
    while (position < length) {
        while (position < length && shell_parser_is_whitespace(input[position])) {
            position++;
        }
        if (position >= length) {
            break;
        }

        if (result->argc >= SHELL_COMMAND_MAX_ARGS) {
            return E_BADCOUNT;
        }

        result->argv[result->argc] = &result->storage[result->storage_used];
        bool in_quotes = false;
        bool token_started = false;

        while (position < length) {
            uint32_t codepoint = input[position];

            if (in_quotes) {
                if (codepoint == (uint32_t)'"') {
                    in_quotes = false;
                    token_started = true;
                    position++;
                    continue;
                }

                if (codepoint == (uint32_t)'\\' && position + 1U < length &&
                    (input[position + 1U] == (uint32_t)'"' ||
                     input[position + 1U] == (uint32_t)'\\')) {
                    codepoint = input[position + 1U];
                    position += 2U;
                } else {
                    position++;
                }

                K_TRY(shell_parser_append_utf8(result, codepoint));
                token_started = true;
                continue;
            }

            if (codepoint == (uint32_t)'"') {
                in_quotes = true;
                token_started = true;
                position++;
                continue;
            }

            if (shell_parser_is_whitespace(codepoint)) {
                position++;
                break;
            }

            position++;
            K_TRY(shell_parser_append_utf8(result, codepoint));
            token_started = true;
        }

        if (in_quotes) {
            return E_INVAL;
        }
        if (!token_started || result->storage_used >= sizeof(result->storage)) {
            return E_BADSIZE;
        }

        result->storage[result->storage_used++] = '\0';
        result->argc++;
    }

    return E_OK;
}
