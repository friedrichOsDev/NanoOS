/**
 * @file serial.h
 * @brief Treiber-Schnittstelle für die serielle UART-Kommunikation (16550/COM-Ports).
 * @details Bietet Makros für Standard-COM-Ports sowie Prototypen zur Initialisierung
 *          und Datenausgabe über die serielle Schnittstelle.
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

#define SERIAL_BUFFER_MAX_SIZE 1024

/**
 * @brief Initialisiert einen UART COM-Port für serielle Übertragung.
 * @details Konfiguriert die Baudrate (115200 Baud), Deaktiviert Interrupts und setzt
 *          die Datenleitungen auf 8N1 (8 Datenbits, keine Parität, 1 Stoppbit).
 * @param port I/O-Port-Adresse des COM-Ports (z. B. COM1/0x3F8).
 */
void serial_init(uint16_t port);

/**
 * @brief Sendet ein einzelnes Zeichen über den angegebenen COM-Port.
 * @param port I/O-Port-Adresse des COM-Ports.
 * @param c Das zu sendende Zeichen.
 */
void serial_putc(uint16_t port, char c);

/**
 * @brief Sendet einen nullterminierten String über den COM-Port.
 * @details Konvertiert Unix-Zeilenumbrüche ('\n') automatisch in CR+LF ('\r\n').
 * @param port I/O-Port-Adresse des COM-Ports.
 * @param str Zeiger auf den auszugebenden String.
 */
void serial_puts(uint16_t port, const char *str);

/**
 * @brief Formatierte Ausgabe über die serielle Schnittstelle (Thread-sicher).
 * @details Nutzt intern vsnprintf und sichert den Zugriff über Spinlocks und IRQ-Save ab.
 * @param port I/O-Port-Adresse des COM-Ports.
 * @param format Formatstring (analog zu printf).
 * @param ... Variable Argumentenliste.
 */
void serial_printf(uint16_t port, const char *format, ...);