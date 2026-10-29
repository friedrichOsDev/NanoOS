/**
 * @file serial.c
 * @brief Implementierung des UART 16550 Serial-Driver.
 * @details Enthält Low-Level-Funktionen zur Port-Ansteuerung, Pufferverwaltung und
 *          synchronisierten formatierte Log-Ausgaben.
 * @author friedrichOsDev
 */

#include "serial.h"
#include <arch/arch.h>
#include <kernel/sync.h>
#include <lib/print.h>
#include <pch.h>

static spinlock_t serial_lock = SPINLOCK_INIT;
static uint16_t port;

static bool serial_is_transmit_empty() {
    return inb((uint16_t)(port + 5)) & SERIAL_LSR_THR_EMPTY;
}

void serial_init() {
    port = COM1;
    outb((uint16_t)(port + 1), 0x00); // Interrupts deaktivieren
    outb((uint16_t)(port + 3), 0x80); // DLAB aktivieren (Baudraten-Divisor setzen)
    outb((uint16_t)(port + 0), 0x01); // Divisor Low Byte (115200 Baud)
    outb((uint16_t)(port + 1), 0x00); // Divisor High Byte
    outb((uint16_t)(port + 3), 0x03); // 8 Bits, keine Parität, 1 Stoppbit (8N1)
    outb((uint16_t)(port + 2), 0xC7); // FIFO aktivieren, Puffer leeren, 14-Byte Schwelle
    outb((uint16_t)(port + 4), 0x0B); // IRQs aktivieren, RTS/DSR Pins setzen

    serial_putc('\n'); // verify that the first log starts on a new line
}

void serial_putc(char c) {
    uint32_t timeout = 100000;
    while (serial_is_transmit_empty() == 0) {
        if (--timeout == 0) {
            return;
        }
    }
    outb(port, (uint8_t)c);
}

void serial_puts(const char *str) {
    while (*str) {
        if (*str == '\n') {
            serial_putc('\r');
        }
        serial_putc(*str++);
    }
}

void vserial_printf(const char *format, va_list args) {
    char buffer[SERIAL_BUFFER_MAX_SIZE];

    uint64_t flags = spinlock_acquire_irqsave(&serial_lock);

    int res = vsnprintf(buffer, sizeof(buffer), format, args);

    if (res > 0) {
        serial_puts(buffer);
    }

    spinlock_release_irqrestore(&serial_lock, flags);
}

void serial_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vserial_printf(format, args);
    va_end(args);
}
