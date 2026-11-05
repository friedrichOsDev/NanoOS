/**
 * @file conv.c
 * @brief Conversion functions implementation.
 * @author friedrichOsDev
 */

#include <lib/util/conv.h>
#include <pch.h>

/**
 * @brief Internal helper function to convert an integer to a string or wide string.
 *
 * @param[out] buffer Target buffer (either `char*` or `uint32_t*`).
 * @param[in] value The numeric value to convert.
 * @param[in] base Base of the number system.
 * @param[in] is_wide `true` for 32-bit wide characters (`uint32_t*`), `false` for ASCII (`char*`).
 * @return Length of the generated string.
 */
static int uint_to_str_internal(void *buffer, uint64_t value, unsigned int base, bool is_wide) {
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
        temp_buf[idx++] = (rem > 9) ? (char)((rem - 10) + 'A') : (char)(rem + '0');
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
 * @brief Converts an unsigned 64-bit integer to a wide string (UTF-32).
 *
 * @param[in] value The numeric value to convert.
 * @param[out] buffer Pointer to the destination wide character buffer (`uint32_t*`).
 * @param[in] base Base of the number system.
 * @return Length of the generated wide string.
 */
int uint_to_str(uint64_t value, uint32_t *buffer, unsigned int base) {
    return uint_to_str_internal(buffer, value, base, true);
}

/**
 * @brief Converts an unsigned 64-bit integer to an ASCII string (`char*`).
 *
 * @param[in] value The numeric value to convert.
 * @param[out] buffer Pointer to the destination string buffer (`char*`).
 * @param[in] base Base of the number system.
 * @return Length of the generated string.
 */
int uint_to_wstr(uint64_t value, char *buffer, unsigned int base) {
    return uint_to_str_internal(buffer, value, base, false);
}

/**
 * @brief Converts a double-precision floating-point number to an ASCII string.
 *
 * @param[in] value The floating-point value to convert.
 * @param[out] buf Pointer to the destination buffer.
 * @param[in] prec Number of decimal places of precision.
 * @return Length of the generated string.
 */
int double_to_str(double value, char *buf, int prec) {
    if (prec < 0)
        prec = 6;
    if (prec > 9)
        prec = 9;

    union {
        double d;
        uint64_t u;
    } pun = {.d = value};

    uint64_t exp_bits = (pun.u >> 52) & 0x7FF;
    uint64_t mant_bits = pun.u & 0x000FFFFFFFFFFFFFULL;

    if (exp_bits == 0x7FF) {
        if (mant_bits != 0) {
            buf[0] = 'n';
            buf[1] = 'a';
            buf[2] = 'n';
            buf[3] = '\0';
            return 3;
        }
        int idx = 0;
        if ((pun.u >> 63) != 0)
            buf[idx++] = '-';
        buf[idx++] = 'i';
        buf[idx++] = 'n';
        buf[idx++] = 'f';
        buf[idx] = '\0';
        return idx;
    }

    int idx = 0;
    if ((pun.u >> 63) != 0) {
        buf[idx++] = '-';
        value = -value;
    }

    uint64_t ipart = (uint64_t)value;
    double fpart = value - (double)ipart;

    double mult = 1.0;
    for (int i = 0; i < prec; i++)
        mult *= 10.0;

    uint64_t fdigits = 0;
    if (prec > 0) {
        fdigits = (uint64_t)(fpart * mult + 0.5);
        if (fdigits >= (uint64_t)mult) {
            ipart++;
            fdigits -= (uint64_t)mult;
        }
    } else {
        if (fpart >= 0.5) {
            ipart++;
        }
    }

    idx += uint_to_wstr(ipart, buf + idx, 10);

    if (prec > 0) {
        buf[idx++] = '.';

        char fbuf[16];
        int flen = uint_to_wstr(fdigits, fbuf, 10);

        for (int i = 0; i < (prec - flen); i++) {
            buf[idx++] = '0';
        }
        for (int i = 0; i < flen; i++) {
            buf[idx++] = fbuf[i];
        }
    }

    buf[idx] = '\0';
    return idx;
}

/**
 * @brief Converts a double-precision floating-point number to a wide string (UTF-32).
 *
 * @param[in] value The floating-point value to convert.
 * @param[out] buf Pointer to the destination wide character buffer.
 * @param[in] prec Number of decimal places of precision.
 * @return Length of the generated wide string.
 */
int double_to_wstr(double value, uint32_t *buf, int prec) {
    char cbuf[64];
    int len = double_to_str(value, cbuf, prec);
    for (int i = 0; i <= len; i++) {
        buf[i] = (uint32_t)(unsigned char)cbuf[i];
    }
    return len;
}
