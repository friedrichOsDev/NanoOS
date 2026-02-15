/*
 * @file serial.h
 * @brief Header file for serial port driver
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define SERIAL_PORT_COM1 0x3F8
#define SERIAL_LSR_DATA_READY 0x01
#define SERIAL_LSR_THR_EMPTY 0x20

void serial_init(void);
void serial_printf(const char *format, ...);
