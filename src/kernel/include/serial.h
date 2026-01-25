/*
 * @file serial.h
 * @brief Header file for serial driver implementation
 * @author friedrichOsDev
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>

#define COM1_PORT 0x3F8

void serial_init(void);
void __attribute__((__target__("no-sse"))) serial_putc(char c);
void __attribute__((__target__("no-sse"))) serial_puts(const char* s);
void __attribute__((__target__("no-sse"))) serial_put_hex(uint32_t value);
void __attribute__((__target__("no-sse"))) serial_put_int(uint32_t value);

#endif // SERIAL_H
