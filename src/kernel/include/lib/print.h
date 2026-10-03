/**
 * @file print.h
 * @brief Printf Implementation
 * @author friedrichOsDev
 */

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Formatiert einen String in ein 32-Bit-Ziel-Array (UTF-32/UCS-4) mit variabler Argumentliste.
 *
 * @param dest Zeiger auf den Zielpuffer für 32-Bit-Zeichen.
 * @param size Maximale Größe des Zielpuffers (inklusive Nullterminierung).
 * @param format Formatstring bestehend aus 32-Bit-Zeichen.
 * @param ... Variable Argumente entsprechend den Format-Spezifizierern.
 * @return Anzahl der Zeichen, die geschrieben worden wären (ohne Nullterminator), analog zu standardmäßigem `snprintf`.
 */
int usnprintf(uint32_t *dest, size_t size, const uint32_t *format, ...);

/**
 * @brief Formatiert einen String in ein 32-Bit-Ziel-Array unter Verwendung eines `va_list`-Arguments.
 *
 * @param dest Zeiger auf den Zielpuffer für 32-Bit-Zeichen.
 * @param size Maximale Größe des Zielpuffers.
 * @param format Formatstring bestehend aus 32-Bit-Zeichen.
 * @param args Initialisierte va_list mit den Format-Argumenten.
 * @return Anzahl der Zeichen, die geschrieben worden wären (ohne Nullterminator).
 */
int uvsnprintf(uint32_t *dest, size_t size, const uint32_t *format, va_list args);

/**
 * @brief Formatiert einen Standard-ASCII/UTF-8-String in einen Puffer mit variabler Argumentliste.
 *
 * @param dest Zeiger auf den Ziel-Puffer (char*).
 * @param size Maximale Größe des Zielpuffers.
 * @param format Formatstring.
 * @param ... Variable Argumente.
 * @return Anzahl der Zeichen, die geschrieben worden wären (ohne Nullterminator).
 */
int snprintf(char *dest, size_t size, const char *format, ...);

/**
 * @brief Formatiert einen Standard-ASCII/UTF-8-String unter Verwendung eines `va_list`-Arguments.
 *
 * @param dest Zeiger auf den Ziel-Puffer (char*).
 * @param size Maximale Größe des Zielpuffers.
 * @param format Formatstring.
 * @param args Initialisierte va_list mit den Format-Argumenten.
 * @return Anzahl der Zeichen, die geschrieben worden wären (ohne Nullterminator).
 */
int vsnprintf(char *dest, size_t size, const char *format, va_list args);