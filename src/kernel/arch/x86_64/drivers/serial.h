/**
 * @file serial.h
 * @brief Internal definitions for the serial module.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define COM1 0x3F8
// #define COM2 0x2F8
// #define COM3 0x3E8
// #define COM4 0x2E8

#define SERIAL_BUFFER_MAX_SIZE 1024
#define SERIAL_LSR_THR_EMPTY 0x20
