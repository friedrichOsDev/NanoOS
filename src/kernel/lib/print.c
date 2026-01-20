/*
 * @file print.c
 * @brief printf implementation
 * @author friedrichOsDev
 */

#include <print.h>
#include <console.h>
#include <fb.h>

/*
 * Helper function to print an integer in decimal format
 * @param value The integer value to print
 */
static void print_int(int value) {
    char buffer[12];
    int i = 0;
    unsigned int uvalue = value;
    int is_negative = 0;

    if (value == 0) {
        console_putc('0');
        return;
    }

    if (value < 0) {
        is_negative = 1;
        uvalue = -value;
    }

    while (uvalue != 0) {
        buffer[i++] = (uvalue % 10) + '0';
        uvalue /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    for (int j = i - 1; j >= 0; j--) {
        console_putc(buffer[j]);
    }
}

/*
 * Helper function to print an unsigned integer in hexadecimal format
 * @param value The unsigned integer value to print
 */
static void print_hex(unsigned int value) {
    char hex_str[8];
    int i;

    for (i = 7; i >= 0; i--) {
        unsigned int digit = value & 0xF;
        if (digit < 10) {
            hex_str[i] = digit + '0';
        } else {
            hex_str[i] = digit - 10 + 'a';
        }
        value >>= 4;
    }

    for (i = 0; i < 8; i++) {
        console_putc(hex_str[i]);
    }
}

/*
 * Variadic printf function to handle formatted output
 * @param format The format string
 * @param args The va_list of arguments
 */
void vprintf(const char* format, va_list args) {
    while (*format != '\0') {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    char* s = va_arg(args, char*);
                    console_puts(s);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    console_putc(c);
                    break;
                }
                case 'd': {
                    int i = va_arg(args, int);
                    print_int(i);
                    break;
                }
                case 'x': {
                    unsigned int i = va_arg(args, unsigned int);
                    print_hex(i);
                    break;
                }
                case 'k': {
                    uint32_t color = va_arg(args, uint32_t);
                    console_set_fg_color(color);
                    break;
                }
                case 'K': {
                    uint32_t color = va_arg(args, uint32_t);
                    console_set_bg_color(color);
                    break;
                }
                case '%': {
                    console_putc('%');
                    break;
                }
                default:
                    console_putc('%');
                    console_putc(*format);
                    break;
            }
        } else {
            console_putc(*format);
        }
        format++;
    }
}

/*
 * Printf function to print formatted output to the console
 * @param format The format string
 * @param ... The arguments to format
 */
void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}