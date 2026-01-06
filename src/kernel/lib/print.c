#include "../include/print.h"
#include "../include/console.h"

void print_int(int value) {
    char buffer[12]; // Enough for 32-bit int
    int i = 0;
    int is_negative = 0;

    if (value == 0) {
        console_putc('0');
        return;
    }

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    while (value != 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    // Reverse the string
    for (int j = i - 1; j >= 0; j--) {
        console_putc(buffer[j]);
    }
}

void print_hex(unsigned int value) {
    char buffer[9]; // Enough for 32-bit hex
    int i = 0;

    if (value == 0) {
        console_putc('0');
        return;
    }

    while (value != 0) {
        unsigned int digit = value % 16;
        if (digit < 10) {
            buffer[i++] = digit + '0';
        } else {
            buffer[i++] = digit - 10 + 'a';
        }
        value /= 16;
    }

    // 0x
    buffer[i++] = 'x';
    buffer[i++] = '0';

    // Reverse the string
    for (int j = i - 1; j >= 0; j--) {
        console_putc(buffer[j]);
    }
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

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
                    char c = (char)va_arg(args, int); // char is promoted to int
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

    va_end(args);
}