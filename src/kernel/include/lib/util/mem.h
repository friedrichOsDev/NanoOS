#pragma once

#include <pch.h>

/**
 * @brief Füllt einen Speicherbereich mit einem bestimmten Byte-Wert.
 *
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param value Wert (Byte), der geschrieben werden soll.
 * @param count Anzahl der zu schreibenden Bytes.
 * @return Zeiger auf den Zielspeicherbereich (dest).
 */
void *memset(void *dest, uint8_t value, size_t count);

/**
 * @brief Kopiert einen Speicherbereich von einer Quelle zu einem Ziel.
 *
 * @note Die Speicherbereiche dürfen sich nicht überlappen.
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param src Zeiger auf den Quellspeicherbereich.
 * @param count Anzahl der zu kopierenden Bytes.
 * @return Zeiger auf den Zielspeicherbereich (dest).
 */
void *memcpy(void *dest, const void *src, size_t count);

/**
 * @brief Füllt einen Speicherbereich mit einem 32-Bit-Wert (DWord).
 *
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param value 32-Bit-Wert, der geschrieben werden soll.
 * @param count Anzahl der zu schreibenden 32-Bit-Elemente.
 */
void memset32(void *dest, uint32_t value, size_t count);

/**
 * @brief Füllt einen Speicherbereich mit einem 64-Bit-Wert (QWord).
 *
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param value 64-Bit-Wert, der geschrieben werden soll.
 * @param count Anzahl der zu schreibenden 64-Bit-Elemente.
 */
void memset64(void *dest, uint64_t value, size_t count);

/**
 * @brief Vergleicht zwei Speicherbereiche Byte für Byte.
 *
 * @param ptr1 Zeiger auf den ersten Speicherbereich.
 * @param ptr2 Zeiger auf den zweiten Speicherbereich.
 * @param count Anzahl der zu vergleichenden Bytes.
 * @return < 0 wenn ptr1 kleiner als ptr2 ist,
 *         > 0 wenn ptr1 größer als ptr2 ist,
 *           0 wenn beide Bereiche identisch sind.
 */
int memcmp(const void *ptr1, const void *ptr2, size_t count);

/**
 * @brief Kopiert einen Speicherbereich auch bei überlappenden Quell- und Zielbereichen sicher.
 *
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param src Zeiger auf den Quellspeicherbereich.
 * @param count Anzahl der zu kopierenden Bytes.
 * @return Zeiger auf den Zielspeicherbereich (dest).
 */
void *memmove(void *dest, const void *src, size_t count);
