/**
 * @file serial.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define SERIAL_PORT_COM1 0x3F8
#define SERIAL_LSR_DATA_READY 0x01
#define SERIAL_LSR_THR_EMPTY 0x20
#define SERIAL_BUFFER_MAX_SIZE 1024

void serial_init();
void serial_printf(const char *format, ...);
