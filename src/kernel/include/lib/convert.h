/**
 * @file convert.h
 * @brief Konvertierungsfunktionen
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/**
 * @brief Konvertiert einen vorzeichenlosen 64-Bit-Ganzzahlwert in einen 32-Bit-String (UTF-32/UCS-4).
 *
 * @param value Der zu konvertierende Wert.
 * @param buffer Zielpuffer für die 32-Bit-Zeichen (muss ausreichend Speicherplatz bieten).
 * @param base Basis der Zahlendarstellung (z. B. 10 für Dezimal, 16 für Hexadezimal, 8 für Oktal, 2 für Binär).
 * @return Länge des erzeugten Strings (ohne Nullterminator).
 */
int uint_to_str(uint64_t value, uint32_t *buffer, int base);

/**
 * @brief Konvertiert einen vorzeichenlosen 64-Bit-Ganzzahlwert in einen Standard-ASCII-String.
 *
 * @param value Der zu konvertierende Wert.
 * @param buffer Zielpuffer für die char-Zeichen (muss ausreichend Speicherplatz bieten).
 * @param base Basis der Zahlendarstellung (z. B. 10 für Dezimal, 16 für Hexadezimal).
 * @return Länge des erzeugten Strings (ohne Nullterminator).
 */
int uint_to_wstr(uint64_t value, char *buffer, int base);

/**
 * @brief Konvertiert eine Gleitkommazahl (double) in einen ASCII-String.
 *
 * @note Behandelt auch Sonderfälle wie NaN und Infinity.
 *
 * @param value Die zu konvertierende Gleitkommazahl.
 * @param buf Zielpuffer für den resultierenden ASCII-String.
 * @param prec Anzahl der Nachkommastellen (Präzision, zwischen 0 und 9).
 * @return Länge des erzeugten Strings (ohne Nullterminator).
 */
int double_to_str(double value, char *buf, int prec);

/**
 * @brief Konvertiert eine Gleitkommazahl (double) in einen 32-Bit-String (UTF-32/UCS-4).
 *
 * @param value Die zu konvertierende Gleitkommazahl.
 * @param buf Zielpuffer für 32-Bit-Zeichen.
 * @param prec Anzahl der Nachkommastellen (Präzision, zwischen 0 und 9).
 * @return Länge des erzeugten Strings (ohne Nullterminator).
 */
int double_to_wstr(double value, uint32_t *buf, int prec);