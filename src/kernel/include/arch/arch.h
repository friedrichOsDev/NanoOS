#pragma once

#include <pch.h>

// serial port on x86_64
void serial_init();
void serial_putc(char_ascii c);
void serial_puts(const char_ascii *str);
void serial_printf(const char_ascii *format, ...);
