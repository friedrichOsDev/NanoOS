/**
 * @file serial.c
 * @author friedrichOsDev
 */

#include <serial.h>
#include <io.h>
#include <print.h>
#include <kernel.h>

/**
 * @brief Initializes the COM1 serial port.
 * 
 * Configures the baud rate to 115200, 8 bits, no parity, and 1 stop bit.
 */
void serial_init(void) {
    outb(SERIAL_PORT_COM1 + 1, 0x00);
    outb(SERIAL_PORT_COM1 + 3, 0x80);
    outb(SERIAL_PORT_COM1 + 0, 0x01);
    outb(SERIAL_PORT_COM1 + 1, 0x00);
    outb(SERIAL_PORT_COM1 + 3, 0x03);
    outb(SERIAL_PORT_COM1 + 2, 0xC7);
    outb(SERIAL_PORT_COM1 + 4, 0x0B);
    init_state = INIT_SERIAL;
}

/**
 * @brief Checks if the transmit holding register is empty.
 *
 * @return true if the transmit buffer is empty and ready for a new character.
 */
static bool serial_is_transmit_empty(void) {
    return inb(SERIAL_PORT_COM1 + 5) & SERIAL_LSR_THR_EMPTY;
}

/**
 * @brief Sends a single character over the serial port.
 *
 * @param c The character to send.
 */
static void serial_putc(char c) {
    while (serial_is_transmit_empty() == 0);
    outb(SERIAL_PORT_COM1, c);
}

/**
 * @brief Sends a null-terminated string over the serial port.
 *
 * @param str The string to send.
 */
static void serial_puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            serial_putc('\r');
        }
        serial_putc(*str++);
    }
}

/**
 * @brief Formatted print to the serial port.
 *
 * @param format The format string.
 * @param ... Additional arguments for the format string.
 */
void serial_printf(const char *format, ...) {
    int res = 0;

    va_list args;
    va_start(args, format);
    res = vsnprintf(NULL, 0, format, args);
    va_end(args);
    
    if (res < 0 || res > SERIAL_BUFFER_MAX_SIZE) return;

    char buffer[res + 1];
    va_start(args, format);
    vsnprintf(buffer, res + 1, format, args);
    va_end(args);

    serial_puts(buffer);
}
