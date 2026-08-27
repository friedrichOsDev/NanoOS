/**
 * @file print.c
 * @brief Printf implementation code
 * @author friedrichOsDev
 */

#include <lib/convert.h>
#include <lib/print.h>
#include <stdbool.h>

/**
 * Optimized print format function for Unicode and Char
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @param args Arguments for the format string.
 * @param is_wide If true: Unicode Else: Char
 * @return The number of characters that would have been written.
 */
static int print_formatted(void *dest, size_t size, const void *format,
                           va_list args, bool is_wide) {
    size_t i = 0;
    size_t fmt_idx = 0;

#define GET_CHAR()                                                             \
    (is_wide ? (uint32_t)((const uint32_t *)format)[fmt_idx]                   \
             : (uint32_t)((const char *)format)[fmt_idx])
#define PUSH_CHAR(c)                                                           \
    do {                                                                       \
        if (dest && i < size - 1) {                                            \
            if (is_wide)                                                       \
                ((uint32_t *)dest)[i] = (uint32_t)(c);                         \
            else                                                               \
                ((char *)dest)[i] = (char)(c);                                 \
        }                                                                      \
        i++;                                                                   \
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
            width = width * 10 + (c - '0');
            c = GET_CHAR();
            fmt_idx++;
        }

        int precision = -1;
        if (c == '.') {
            c = GET_CHAR();
            fmt_idx++;
            precision = 0;
            while (c >= '0' && c <= '9') {
                precision = precision * 10 + (c - '0');
                c = GET_CHAR();
                fmt_idx++;
            }
        }

        int length_mod = 0; // 0=default, 1=l, 2=ll, 3=z, 4=h, 5=hh
        if (c == 'l') {
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
                val_d = va_arg(args, size_t);
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
            int len = is_wide ? uint_to_str(abs_val, w_buf, 10)
                              : uint_to_str_legacy(abs_val, a_buf, 10);
            int pad = (width > len) ? (width - len) : 0;

            if (!r_width && !pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR(' ');
            if (!r_width && pad_zero)
                while (pad-- > 0)
                    PUSH_CHAR('0');

            for (int j = 0; j < len; j++)
                PUSH_CHAR(is_wide ? w_buf[j]
                                  : (uint32_t)(unsigned char)a_buf[j]);
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

            int base = (c == 'u') ? 10 : ((c == 'o') ? 8 : 16);
            uint32_t w_buf[64];
            char a_buf[64];
            int len = is_wide ? uint_to_str(val_u, w_buf, base)
                              : uint_to_str_legacy(val_u, a_buf, base);

            int prefix_len = (c == 'x' || c == 'X' || c == 'o') ? 2 : 0;
            int pad =
                (width > (len + prefix_len)) ? (width - (len + prefix_len)) : 0;

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
                PUSH_CHAR(is_wide ? w_buf[j]
                                  : (uint32_t)(unsigned char)a_buf[j]);
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
            int len = is_wide ? uint_to_str(val_p, w_buf, 16)
                              : uint_to_str_legacy(val_p, a_buf, 16);
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
                PUSH_CHAR(is_wide ? w_buf[j]
                                  : (uint32_t)(unsigned char)a_buf[j]);
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
 * Formatted print to a fixed-size buffer.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @param ... Arguments for the format string.
 * @return The number of characters that would have been written.
 */
int usnprintf(uint32_t *dest, const size_t size, const uint32_t *format, ...) {
    va_list args;
    va_start(args, format);
    const int res = print_formatted(dest, size, format, args, true);
    va_end(args);
    return res;
}

/**
 * Formatted print to a fixed-size buffer using a va_list.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @param args The list of arguments.
 * @return The number of characters that would have been written.
 */
int uvsnprintf(uint32_t *dest, size_t size, const uint32_t *format,
               va_list args) {
    return print_formatted(dest, size, format, args, true);
}

/**
 * @brief Formatted print to a fixed-size buffer.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @param ... Arguments for the format string.
 * @return The number of characters that would have been written.
 */
int snprintf(char *dest, const size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int res = print_formatted(dest, size, format, args, false);
    va_end(args);
    return res;
}

/**
 * @brief Formatted print to a fixed-size buffer using a va_list.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @param args The list of arguments.
 * @return The number of characters that would have been written.
 */
int vsnprintf(char *dest, size_t size, const char *format, va_list args) {
    return print_formatted(dest, size, format, args, false);
}