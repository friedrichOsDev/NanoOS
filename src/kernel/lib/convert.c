/**
 * @file convert.c
 * @brief Convertion Functions
 * @author friedrichOsDev
 */

#include <lib/convert.h>
#include <stdbool.h>

/**
 * Converts an unsigned 64-bit integer to a string (Internal)
 * @param buffer The destination buffer
 * @param value The value to convert
 * @param base The numerical base (e.g., 10 for decimal, 16 for hex)
 * @param is_wide If true: Unicode Else: Char
 * @return The length of the resulting string
 */
static int uint_to_str_internal(void *buffer, uint64_t value, int base,
                                bool is_wide) {
    char temp_buf[66];
    int idx = 0;

    if (value == 0) {
        if (is_wide) {
            ((uint32_t *)buffer)[0] = U'0';
            ((uint32_t *)buffer)[1] = U'\0';
        } else {
            ((char *)buffer)[0] = '0';
            ((char *)buffer)[1] = '\0';
        }
        return 1;
    }

    while (value != 0) {
        const uint64_t rem = value % base;
        value /= base;
        temp_buf[idx++] =
            (rem > 9) ? (char)((rem - 10) + 'A') : (char)(rem + '0');
    }

    const int len = idx;
    for (int i = 0; i < len; i++) {
        const char digit = temp_buf[len - 1 - i];
        if (is_wide) {
            ((uint32_t *)buffer)[i] = (uint32_t)digit;
        } else {
            ((char *)buffer)[i] = digit;
        }
    }

    if (is_wide) {
        ((uint32_t *)buffer)[len] = U'\0';
    } else {
        ((char *)buffer)[len] = '\0';
    }

    return len;
}

/**
 * Converts an unsigned 64-bit integer to a string
 * @param value The value to convert
 * @param buffer The destination buffer
 * @param base The numerical base (e.g., 10 for decimal, 16 for hex)
 * @return The length of the resulting string
 */
int uint_to_str(uint64_t value, uint32_t *buffer, int base) {
    return uint_to_str_internal(buffer, value, base, true);
}

/**
 * Converts an unsigned 64-bit integer to a string (legacy)
 * @param value The value to convert
 * @param buffer The destination buffer
 * @param base The numerical base (e.g., 10 for decimal, 16 for hex)
 * @return The length of the resulting string
 */
int uint_to_str_legacy(uint64_t value, char *buffer, int base) {
    return uint_to_str_internal(buffer, value, base, false);
}