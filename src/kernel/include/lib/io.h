/**
 * @file io.h
 * @brief I/O Port Implementation
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/**
 * @brief Schreibt ein Byte (8 Bit) an einen I/O-Port.
 *
 * @param port Adresse des Ziel-I/O-Ports.
 * @param val Der zu schreibende 8-Bit-Wert.
 */
void outb(uint16_t port, uint8_t val);

/**
 * @brief Liest ein Byte (8 Bit) von einem I/O-Port.
 *
 * @param port Adresse des Quell-I/O-Ports.
 * @return Gelesener 8-Bit-Wert.
 */
uint8_t inb(uint16_t port);

/**
 * @brief Schreibt ein Word (16 Bit) an einen I/O-Port.
 *
 * @param port Adresse des Ziel-I/O-Ports.
 * @param val Der zu schreibende 16-Bit-Wert.
 */
void outw(uint16_t port, uint16_t val);

/**
 * @brief Liest ein Word (16 Bit) von einem I/O-Port.
 *
 * @param port Adresse des Quell-I/O-Ports.
 * @return Gelesener 16-Bit-Wert.
 */
uint16_t inw(uint16_t port);

/**
 * @brief Schreibt ein Doubleword (32 Bit) an einen I/O-Port.
 *
 * @param port Adresse des Ziel-I/O-Ports.
 * @param val Der zu schreibende 32-Bit-Wert.
 */
void outl(uint16_t port, uint32_t val);

/**
 * @brief Liest ein Doubleword (32 Bit) von einem I/O-Port.
 *
 * @param port Adresse des Quell-I/O-Ports.
 * @return Gelesener 32-Bit-Wert.
 */
uint32_t inl(uint16_t port);