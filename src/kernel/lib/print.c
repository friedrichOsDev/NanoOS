/**
 * @file print.c
 * @brief String formatting implementation.
 * @author friedrichOsDev
 */

#include <lib/print.h>
#include <lib/util/conv.h>
#include <pch.h>

/**
 * @brief Core internal formatting function for processing and formatting strings.
 * 
 * Handles formatting specifiers like width, precision, padding, length modifiers, 
 * and various data types (integers, floats, strings, pointers) for both 8-bit ASCII 
 * and 32-bit wide character buffers.
 * 
 * @param[out] dest Pointer to the destination buffer (char* or uint32_t*).
 * @param[in] size Maximum size of the destination buffer in characters.
 * @param[in] format Format string (const char* or const uint32_t*).
 * @param[in] args Variable argument list matching the format specifiers.
 * @param[in] is_wide `true` for 32-bit wide characters (UTF-32), `false` for standard 8-bit characters.
 * @return int Total number of characters processed or generated (excluding null-terminator).
 */
static int print_formatted(void *dest, size_t size, const void *format, va_list args, bool is_wide) {
    size_t i = 0;
    size_t fmt_idx = 0;

#define GET_CHAR() (is_wide ? (uint32_t)((const uint32_t *)format)[fmt_idx] : (uint32_t)((const char *)format)[fmt_idx])
#define PUSH_CHAR(c)                                   \
    do {                                               \
        if (dest && i < size - 1) {                    \
            if (is_wide)                               \
                ((uint32_t *)dest)[i] = (uint32_t)(c); \
            else                                       \
                ((char *)dest)[i] = (char)(c);         \
        }                                              \
        i++;                                           \
    } while (0)

    uint32_t c;
    while ((c = GET_CHAR()) != 0) {
        fmt_idx++;
        if (c != '%') {
            PUSH_CHAR(c);
            continue;
        }

        c = GET_CHAR();
        fmt_idx++;

        bool pad_zero = false;
        if (c == '0') {
            pad_zero = true;
            c = GET_CHAR();
            fmt_idx++;
        }

        bool r_width = false;
        if (c == '-') {
            r_width = true;
            c = GET_CHAR();
            fmt_idx++;
        }

        int width = 0;
        while (c >= '0' && c <= '9') {
            width = width * 10 + (int)(c - '0');
            c = GET_CHAR();
            fmt_idx++;
        }

        int precision = -1;
        if (c == '.') {
            c = GET_CHAR();
            fmt_idx++;
            precision = 0;
            while (c >= '0' && c <= '9') {
                precision = precision * 10 + (int)(c - '0');
                c = GET_CHAR();
                fmt_idx++;
            }
        }

        int length_mod = 0; // 0=default, 1=l, 2=ll, 3=z, 4=h, 5=hh, 6=L
        if (c == 'L') {
            length_mod = 6;
            c = GET_CHAR();
            fmt_idx++;
        } else if (c == 'l') {
            length_mod = 1;
            c = GET_CHAR();
            fmt_idx++;
            if (c == 'l') {
                length_mod = 2;
                c = GET_CHAR();
                fmt_idx++;
            }
        } else if (c == 'z') {
            length_mod = 3;
            c = GET_CHAR();
            fmt_idx++;
        } else if (c == 'h') {
            length_mod = 4;
            c = GET_CHAR();
            fmt_idx++;
            if (c == 'h') {
                length_mod = 5;
                c = GET_CHAR();
                fmt_idx++;
            }
        }

        switch (c) {
        case 'd':
        case 'i': {
            int64_t val_d;
            if (length_mod == 2 || length_mod == 1)
                val_d = va_arg(args, long);
            else if (length_mod == 3)
                val_d = (int64_t)va_arg(args, size_t);
            else if (length_mod == 4)
                val_d = (int16_t)va_arg(args, int);
            else if (length_mod == 5)
                val_d = (int8_t)va_arg(args, int);
            else
                val_d = va_arg(args, int);

            uint64_t abs_val;
            if (val_d < 0) {
                PUSH_CHAR('-');
                abs_val = (uint64_t) - (val_d + 1) + 1;
            } else {
                abs_val = (uint64_t)val_d;
            }

            uint32_t w_buf[64];
            char a_buf[64];
            int len = (int)(is_wide ? uint_to_str(abs_val, w_buf, 10) : uint_to_wstr(abs_val, a_buf, 10));
            int pad = (width > len) ? (width - len) : 0;

            if (!r_width && !pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            if (!r_width && pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR('0');

            for (int j = 0; j < len; j++)
                PUSH_CHAR(is_wide ? w_buf[j] : (uint32_t)(unsigned char)a_buf[j]);
            if (r_width)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            break;
        }

        case 'u':
        case 'x':
        case 'X':
        case 'o': {
            uint64_t val_u;
            if (length_mod == 2 || length_mod == 1)
                val_u = va_arg(args, unsigned long);
            else if (length_mod == 3)
                val_u = va_arg(args, size_t);
            else if (length_mod == 4)
                val_u = (uint16_t)va_arg(args, unsigned int);
            else if (length_mod == 5)
                val_u = (uint8_t)va_arg(args, unsigned int);
            else
                val_u = va_arg(args, unsigned int);

            unsigned int base = (c == 'u') ? 10 : ((c == 'o') ? 8 : 16);
            uint32_t w_buf[64];
            char a_buf[64];
            int len = (int)(is_wide ? uint_to_str(val_u, w_buf, base) : uint_to_wstr(val_u, a_buf, base));

            int prefix_len = (c == 'x' || c == 'X' || c == 'o') ? 2 : 0;
            int pad = (width > (len + prefix_len)) ? (width - (len + prefix_len)) : 0;

            if (!r_width && !pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR(' ');

            if (c == 'x' || c == 'X') {
                PUSH_CHAR('0');
                PUSH_CHAR(c);
            } else if (c == 'o') {
                PUSH_CHAR('0');
                PUSH_CHAR('o');
            }

            if (!r_width && pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR('0');

            for (int j = 0; j < len; j++)
                PUSH_CHAR(is_wide ? w_buf[j] : (uint32_t)(unsigned char)a_buf[j]);
            if (r_width)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            break;
        }

        case 'f': {
            double val_f;
            if (length_mod == 6) {
                val_f = (double)va_arg(args, long double);
            } else {
                val_f = va_arg(args, double);
            }

            int p = (precision >= 0) ? precision : 6;
            uint32_t w_buf[64];
            char a_buf[64];
            int len = (int)(is_wide ? double_to_wstr(val_f, w_buf, p) : double_to_str(val_f, a_buf, p));
            int pad = (width > len) ? (width - len) : 0;

            if (!r_width && !pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            if (!r_width && pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR('0');

            for (int j = 0; j < len; j++)
                PUSH_CHAR(is_wide ? w_buf[j] : (uint32_t)(unsigned char)a_buf[j]);
            if (r_width)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            break;
        }

        case 's': {
            int pad;
            if (is_wide) {
                const uint32_t *s = va_arg(args, const uint32_t *);
                if (!s)
                    s = U"(null)";
                int len = 0;
                while (s[len] != 0 && (precision < 0 || len < precision))
                    len++;
                pad = (width > len) ? (width - len) : 0;

                if (!r_width)
                    while (pad-- > 0)
                        PUSH_CHAR(' ');
                for (int j = 0; j < len; j++)
                    PUSH_CHAR(s[j]);
            } else {
                const char *s = va_arg(args, const char *);
                if (!s)
                    s = "(null)";
                int len = 0;
                while (s[len] != '\0' && (precision < 0 || len < precision))
                    len++;
                pad = (width > len) ? (width - len) : 0;

                if (!r_width)
                    while (pad-- > 0)
                        PUSH_CHAR(' ');
                for (int j = 0; j < len; j++)
                    PUSH_CHAR(s[j]);
            }
            if (r_width)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            break;
        }

        case 'c': {
            PUSH_CHAR(va_arg(args, int));
            break;
        }

        case 'p': {
            uintptr_t val_p = va_arg(args, uintptr_t);
            uint32_t w_buf[64];
            char a_buf[64];
            int len = (int)(is_wide ? uint_to_str(val_p, w_buf, 16) : uint_to_wstr(val_p, a_buf, 16));
            int pad = (width > len + 2) ? (width - (len + 2)) : 0;

            if (!r_width && !pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            PUSH_CHAR('0');
            PUSH_CHAR('x');
            if (!r_width && pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR('0');

            for (int j = 0; j < len; j++)
                PUSH_CHAR(is_wide ? w_buf[j] : (uint32_t)(unsigned char)a_buf[j]);
            if (r_width)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            break;
        }

        case '%': {
            PUSH_CHAR('%');
            break;
        }

        default: {
            PUSH_CHAR('%');
            PUSH_CHAR(c);
            break;
        }
        }
    }

    if (dest && size > 0) {
        size_t last = (i < size) ? i : size - 1;
        if (is_wide)
            ((uint32_t *)dest)[last] = 0;
        else
            ((char *)dest)[last] = '\0';
    }

#undef GET_CHAR
#undef PUSH_CHAR
    return (int)i;
}

/**
 * @brief Formats and stores a series of characters in a wide-character buffer (UTF-32).
 * 
 * @param[out] dest Pointer to the destination wide-character buffer.
 * @param[in] size Maximum number of wide characters to write.
 * @param[in] format Wide format string (UTF-32).
 * @param[in] ... Variable arguments for the format string.
 * @return int Number of characters written or required.
 */
int usnprintf(uint32_t *dest, const size_t size, const uint32_t *format, ...) {
    va_list args;
    va_start(args, format);
    const int res = print_formatted(dest, size, format, args, true);
    va_end(args);
    return res;
}

/**
 * @brief Formats and stores a series of characters in a wide-character buffer using a va_list.
 * 
 * @param[out] dest Pointer to the destination wide-character buffer.
 * @param[in] size Maximum number of wide characters to write.
 * @param[in] format Wide format string (UTF-32).
 * @param[in] args Variable argument list (`va_list`).
 * @return int Number of characters written or required.
 */
int uvsnprintf(uint32_t *dest, size_t size, const uint32_t *format, va_list args) {
    return print_formatted(dest, size, format, args, true);
}

/**
 * @brief Formats and stores a series of characters in a standard string buffer.
 * 
 * @param[out] dest Pointer to the destination string buffer.
 * @param[in] size Maximum number of bytes to write.
 * @param[in] format Standard format string.
 * @param[in] ... Variable arguments for the format string.
 * @return int Number of characters written or required.
 */
int snprintf(char *dest, const size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int res = print_formatted(dest, size, format, args, false);
    va_end(args);
    return res;
}

/**
 * @brief Formats and stores a series of characters in a standard string buffer using a va_list.
 * 
 * @param[out] dest Pointer to the destination string buffer.
 * @param[in] size Maximum number of bytes to write.
 * @param[in] format Standard format string.
 * @param[in] args Variable argument list (`va_list`).
 * @return int Number of characters written or required.
 */
int vsnprintf(char *dest, size_t size, const char *format, va_list args) {
    return print_formatted(dest, size, format, args, false);
}
