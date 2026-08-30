/**
 * @file serial.c
 * @brief x86_64 Serial Driver for debugging
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <core/sync.h>
#include <lib/io.h>
#include <lib/print.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_LSR_THR_EMPTY 0x20

static spinlock_t serial_lock = SPINLOCK_INIT;

/**
 * Checks if the serial transmit is empty
 * @param port The COM Port
 * @return True if Empty, False if not
 */
static bool serial_is_transmit_empty(uint16_t port) {
    return inb(port + 5) & SERIAL_LSR_THR_EMPTY;
}

/**
 * Initializes the Serial Driver for a specific COM Port
 * @param port The COM Port for the debugging
 */
void serial_init(uint16_t port) {
    outb(port + 1, 0x00);
    outb(port + 3, 0x80);
    outb(port + 0, 0x01);
    outb(port + 1, 0x00);
    outb(port + 3, 0x03);
    outb(port + 2, 0xC7);
    outb(port + 4, 0x0B);
}

/**
 * Puts a char to the debugging Port
 * @param port The COM Port
 * @param c The Character
 */
void serial_putc(uint16_t port, char c) {
    uint32_t timeout = 100000;
    while (serial_is_transmit_empty(port) == 0) {
        if (--timeout == 0) {
            return;
        }
    }
    outb(port, c);
}

/**
 * Puts a string to the debugging Port
 * @param port The COM Port
 * @param str The String
 */
void serial_puts(uint16_t port, const char *str) {
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

    uint64_t flags = spinlock_acquire_irqsave(&serial_lock);

    va_list args;
    va_start(args, format);
    int res = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (res > 0) {
        serial_puts(port, buffer);
    }

    spinlock_release_irqrestore(&serial_lock, flags);
}