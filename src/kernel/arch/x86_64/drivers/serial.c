/**
 * @file serial.c
 * @brief
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
 * @brief Prüft, ob der Sende-Puffer (Transmit Holding Register) des UARTs leer ist.
 * @param port I/O-Port-Adresse des COM-Ports.
 * @return true, wenn das Register bereit für neue Daten ist, sonst false.
 */
static bool serial_is_transmit_empty(uint16_t port) {
    return inb(port + 5) & SERIAL_LSR_THR_EMPTY;
}

void serial_init(uint16_t port) {
    outb(port + 1, 0x00); // Interrupts deaktivieren
    outb(port + 3, 0x80); // DLAB aktivieren (Baudraten-Divisor setzen)
    outb(port + 0, 0x01); // Divisor Low Byte (115200 Baud)
    outb(port + 1, 0x00); // Divisor High Byte
    outb(port + 3, 0x03); // 8 Bits, keine Parität, 1 Stoppbit (8N1)
    outb(port + 2, 0xC7); // FIFO aktivieren, Puffer leeren, 14-Byte Schwelle
    outb(port + 4, 0x0B); // IRQs aktivieren, RTS/DSR Pins setzen
}

void serial_putc(uint16_t port, char c) {
    uint32_t timeout = 100000;
    while (serial_is_transmit_empty(port) == 0) {
        if (--timeout == 0) {
            return;
        }
    }
    outb(port, c);
}

void serial_puts(uint16_t port, const char *str) {
    while (*str) {
        if (*str == '\n') {
            serial_putc(port, '\r');
        }
        serial_putc(port, *str++);
    }
}

void serial_printf(uint16_t port, const char *format, ...) {
    char buffer[SERIAL_BUFFER_MAX_SIZE];

    // Spinlock akquirieren und Interrupts sichern (Thread- & Interrupt-Sicherheit)
    uint64_t flags = spinlock_acquire_irqsave(&serial_lock);

    va_list args;
    va_start(args, format);
    int res = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (res > 0) {
        serial_puts(port, buffer);
    }

    // Spinlock freigeben und vorherigen Interrupt-Status wiederherstellen
    spinlock_release_irqrestore(&serial_lock, flags);
}