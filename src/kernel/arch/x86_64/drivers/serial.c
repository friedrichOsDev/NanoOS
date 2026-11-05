/**
 * @file serial.c
 * @brief Serial module for debugging and logging.
 * @author friedrichOsDev
 */

#include "serial.h"
#include <arch/arch.h>
#include <kernel/sync.h>
#include <lib/print.h>
#include <pch.h>

static spinlock_t serial_lock = SPINLOCK_INIT;
static uint16_t port;

/**
 * @brief Checks if the serial transmit holding register is empty.
 * 
 * @return true if the transmission buffer is empty and ready for new data, false otherwise.
 */
static bool serial_is_transmit_empty() {
    return inb((uint16_t)(port + 5)) & SERIAL_LSR_THR_EMPTY;
}

/** @brief Initializes the COM1 serial port with standard baud rate, data format, and FIFO settings. */
void serial_init() {
    port = COM1;                      // COM1 = 0x3F8
    outb((uint16_t)(port + 1), 0x00); // Disable interrupts
    outb((uint16_t)(port + 3), 0x80); // Enable DLAB (set baud rate divisor)
    outb((uint16_t)(port + 0), 0x01); // Divisor Low Byte (115200 baud)
    outb((uint16_t)(port + 1), 0x00); // Divisor High Byte
    outb((uint16_t)(port + 3), 0x03); // 8 bits, no parity, 1 stop bit (8N1)
    outb((uint16_t)(port + 2), 0xC7); // Enable FIFO, clear buffer, 14-byte threshold
    outb((uint16_t)(port + 4), 0x0B); // Enable IRQs, set RTS/DSR pins

    serial_putc('\n'); // verify that the first log starts on a new line
    // if serial_putc() fails, the kernel cannot detect it because serial communication is the only debug method
}

/**
 * @brief Writes a single character to the serial port with a timeout safety check.
 * 
 * @param[in] c Character to transmit.
 * @return E_OK on success, or E_TIMEOUT if the transmit buffer did not clear in time.
 */
kerr_t serial_putc(char c) {
    uint32_t timeout = 100000;
    while (serial_is_transmit_empty() == 0) {
        if (--timeout == 0) {
            return E_TIMEOUT;
        }
        arch_pause();
    }
    outb(port, (uint8_t)c);
    return E_OK;
}

/**
 * @brief Writes a null-terminated string to the serial port, automatically translating newlines to CRLF.
 * 
 * @param[in] str Pointer to the null-terminated string.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t serial_puts(const char *str) {
    if (!str)
        return E_INVAL;

    irq_flags_t flags = spinlock_acquire_irqsave(&serial_lock);
    kerr_t err = E_OK;

    while (*str) {
        if (*str == '\n') {
            err = serial_putc('\r');
            if (k_failed(err))
                break;
        }
        err = serial_putc(*str++);
        if (k_failed(err))
            break;
    }

    spinlock_release_irqrestore(&serial_lock, flags);
    return err;
}

/**
 * @brief Formats and prints a string to the serial port using a variable argument list.
 * 
 * @param[in] format Format string conforming to standard formatting rules.
 * @param[in] args Initialized va_list of arguments.
 */
void vserial_printf(const char *format, va_list args) {
    char buffer[SERIAL_BUFFER_MAX_SIZE];

    int res = vsnprintf(buffer, sizeof(buffer), format, args);
    if (res > 0) {
        serial_puts(buffer);
        // if serial_puts(buffer) fails, the kernel cannot detect it because serial communication is the only debug method
    }
}

/**
 * @brief Formats and prints a string to the serial port using variable arguments.
 * 
 * @param[in] format Format string conforming to standard formatting rules.
 * @param[in] ... Additional arguments corresponding to the format string.
 */
void serial_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vserial_printf(format, args);
    va_end(args);
}
