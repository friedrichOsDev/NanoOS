/**
 * @file serial.h
 * @brief x86_64 serial driver for debugging (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

#define SERIAL_BUFFER_MAX_SIZE 1024

void serial_init(uint16_t port);
void serial_putc(uint16_t port, char c);
void serial_puts(uint16_t port, const char *str);
void serial_printf(uint16_t port, const char *format, ...);
