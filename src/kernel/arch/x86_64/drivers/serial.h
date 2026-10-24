/**
 * @file serial.h
 * @brief Treiber-Schnittstelle für die serielle UART-Kommunikation (16550/COM-Ports).
 * @details Bietet Makros für Standard-COM-Ports sowie Prototypen zur Initialisierung
 *          und Datenausgabe über die serielle Schnittstelle.
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
