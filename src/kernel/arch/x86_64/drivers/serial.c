/**
 * @file serial.c
 * @brief x86_64 serial driver for debugging
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <lib/print.h>
#include <lib/io.h>
#include <stdbool.h>
#include <stdarg.h>

#define SERIAL_LSR_THR_EMPTY 0x20

void serial_init(uint16_t port) {
    outb(port + 1, 0x00);
    outb(port + 3, 0x80);
    outb(port + 0, 0x01);
    outb(port + 1, 0x00);
    outb(port + 3, 0x03);
    outb(port + 2, 0xC7);
    outb(port + 4, 0x0B);
}

static bool serial_is_transmit_empty(uint16_t port) {
    return inb(port + 5) & SERIAL_LSR_THR_EMPTY;
}

void serial_putc(uint16_t port, char c) {
    while (serial_is_transmit_empty(port) == 0);
    outb(port, c);
}

void serial_puts(uint16_t port, const char* str) {
    while (*str) {
        if (*str == '\n') {
            serial_putc(port, '\r');
        }
        serial_putc(port, *str++);
    }
}

/**
 * Formatted print to a serial port.
 * @param port Serial Port to print to.
 * @param format The format string.
 * @param ... Arguments for the format string.
 */
void serial_printf(uint16_t port, const char *format, ...) {
    char buffer[SERIAL_BUFFER_MAX_SIZE];

    va_list args;
    va_start(args, format);
    int res = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (res > 0) {
        serial_puts(port, buffer);
    }
}