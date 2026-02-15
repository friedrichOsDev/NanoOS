/*
 * @file serial.c
 * @brief Serial port driver
 * @author friedrichOsDev
 */

#include <serial.h>
#include <io.h>
#include <convert.h>

/*
 * Initializes the serial port
 * @param void
 */
void serial_init(void) {
    outb(SERIAL_PORT_COM1 + 1, 0x00);
    outb(SERIAL_PORT_COM1 + 3, 0x80);
    outb(SERIAL_PORT_COM1 + 0, 0x01);
    outb(SERIAL_PORT_COM1 + 1, 0x00);
    outb(SERIAL_PORT_COM1 + 3, 0x03);
    outb(SERIAL_PORT_COM1 + 2, 0xC7);
    outb(SERIAL_PORT_COM1 + 4, 0x0B);
}

/*
 * Check if the transmit buffer is empty
 * @param void
 * @return true if the transmit buffer is empty, false otherwise
 */
static bool serial_is_transmit_empty(void) {
    return inb(SERIAL_PORT_COM1 + 5) & SERIAL_LSR_THR_EMPTY;
}

/*
 * Write a character to the serial port
 * @param c The character to write
 */
static void serial_putc(char c) {
    while (serial_is_transmit_empty() == 0);
    outb(SERIAL_PORT_COM1, c);
}

/*
 * Write a string to the serial port
 * @param str The string to write
 */
static void serial_puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            serial_putc('\r');
        }
        serial_putc(*str++);
    }
}

/*
 * Write a integer to the serial port
 * @param value The integer to write
 */
static void serial_puti(int32_t value) {
    serial_puts(int_to_str(value));
}

/*
 * Write a hexadecimal value to the serial port
 * @param value The hexadecimal value to write
 */
static void serial_puth(uint32_t value) {
    serial_puts(hex_to_str(value));
}

/*
 * Write a formatted string to the serial port
 * @param format The format string
 * @param ... The arguments to format
 */
void serial_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    for (const char *p = format; *p != '\0'; p++) {
        if (*p != '%') {
            if (*p == '\n') {
                serial_putc('\r');
            }
            serial_putc(*p);
            continue;
        }

        p++;
        switch (*p) {
            case 'c':
                serial_putc((char)va_arg(args, int));
                break;
            case 's':
                serial_puts(va_arg(args, const char*));
                break;
            case 'd':
            case 'i':
                serial_puti(va_arg(args, int32_t));
                break;
            case 'x':
            case 'p':
            case 'X':
                serial_puth(va_arg(args, uint32_t));
                break;
            case '%':
                serial_putc('%');
                break;
            default:
                serial_putc('%');
                serial_putc(*p);
                break;
        }
    }

    va_end(args);
}