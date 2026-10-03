/**
 * @file string.h
 * @brief String- und Speicherfunktionen
 * @author friedrichOsDev
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

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
 * @brief Kopiert 32-Bit-Elemente von einer Quelle zu einem Ziel.
 *
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param src Zeiger auf den Quellspeicherbereich.
 * @param count Anzahl der zu kopierenden 32-Bit-Elemente.
 */
void memcpy32(void *dest, const void *src, size_t count);

/**
 * @brief Füllt einen Speicherbereich mit einem 64-Bit-Wert (QWord).
 *
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param value 64-Bit-Wert, der geschrieben werden soll.
 * @param count Anzahl der zu schreibenden 64-Bit-Elemente.
 */
void memset64(void *dest, uint64_t value, size_t count);

/**
 * @brief Kopiert 64-Bit-Elemente von einer Quelle zu einem Ziel.
 *
 * @param dest Zeiger auf den Zielspeicherbereich.
 * @param src Zeiger auf den Quellspeicherbereich.
 * @param count Anzahl der zu kopierenden 64-Bit-Elemente.
 */
void memcpy64(void *dest, const void *src, size_t count);

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

/**
 * @brief Ermittelt die Länge eines Null-terminierten Strings.
 *
 * @param str Zeiger auf den String.
 * @return Anzahl der Zeichen exklusive Nullbyte.
 */
size_t strlen(const char *str);

/**
 * @brief Kopiert einen String inklusive Nullbyte.
 *
 * @param dest Zielpuffer.
 * @param src Quellstring.
 * @return Zeiger auf den Zielpuffer (dest).
 */
char *strcpy(char *dest, const char *src);

/**
 * @brief Vergleicht zwei Null-terminierte Strings lexikografisch.
 *
 * @param s1 Erster String.
 * @param s2 Zweiter String.
 * @return < 0 wenn s1 kleiner als s2 ist,
 *         > 0 wenn s1 größer als s2 ist,
 *           0 wenn beide Strings identisch sind.
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief Vergleicht maximal n Zeichen zweier Strings.
 *
 * @param s1 Erster String.
 * @param s2 Zweiter String.
 * @param n Maximale Anzahl der zu vergleichenden Zeichen.
 * @return < 0 wenn s1 kleiner als s2 ist,
 *         > 0 wenn s1 größer als s2 ist,
 *           0 wenn die ersten n Zeichen identisch sind.
 */
int strncmp(const char *s1, const char *s2, size_t n);

/**
 * @brief Kopiert maximal n Zeichen eines Strings. Füllt mit Nullbytes auf, falls src kürzer ist.
 *
 * @param dest Zielpuffer.
 * @param src Quellstring.
 * @param n Maximale Anzahl der zu kopierenden Zeichen.
 * @return Zeiger auf den Zielpuffer (dest).
 */
char *strncpy(char *dest, const char *src, size_t n);

/**
 * @brief Hängt den Quellstring an den Zielstring an.
 *
 * @param dest Zielstring (muss ausreichend Speicherplatz bieten).
 * @param src Quellstring.
 * @return Zeiger auf den Zielstring (dest).
 */
char *strcat(char *dest, const char *src);

/**
 * @brief Erstellt eine Duplikatskopie eines Strings auf dem Heap.
 *
 * @param src Quellstring.
 * @return Zeiger auf den neu allokierten String oder NULL bei Fehler.
 */
char *strdup(const char *src);

/**
 * @brief Ermittelt die Länge eines Null-terminierten 32-Bit-Strings (UTF-32/UCS-4).
 *
 * @param str Zeiger auf den 32-Bit-String.
 * @return Anzahl der 32-Bit-Zeichen exklusive Terminations-Null.
 */
size_t u32_strlen(const uint32_t *str);

/**
 * @brief Kopiert einen 32-Bit-String.
 *
 * @param dest Zielpuffer für 32-Bit-Zeichen.
 * @param src Quellstring mit 32-Bit-Zeichen.
 * @return Zeiger auf den Zielpuffer (dest).
 */
uint32_t *u32_strcpy(uint32_t *dest, const uint32_t *src);

/**
 * @brief Vergleicht zwei 32-Bit-Strings lexikografisch.
 *
 * @param s1 Erster 32-Bit-String.
 * @param s2 Zweiter 32-Bit-String.
 * @return -1 wenn s1 < s2, 1 wenn s1 > s2, 0 bei Gleichheit.
 */
int u32_strcmp(const uint32_t *s1, const uint32_t *s2);

/**
 * @brief Vergleicht maximal n Zeichen zweier 32-Bit-Strings.
 *
 * @param s1 Erster 32-Bit-String.
 * @param s2 Zweiter 32-Bit-String.
 * @param n Maximale Anzahl der zu vergleichenden Zeichen.
 * @return -1 wenn s1 < s2, 1 wenn s1 > s2, 0 bei Gleichheit.
 */
int u32_strncmp(const uint32_t *s1, const uint32_t *s2, size_t n);

/**
 * @brief Kopiert maximal n Zeichen eines 32-Bit-Strings.
 *
 * @param dest Zielpuffer.
 * @param src Quellstring.
 * @param n Maximale Anzahl der zu kopierenden Zeichen.
 * @return Zeiger auf den Zielpuffer (dest).
 */
uint32_t *u32_strncpy(uint32_t *dest, const uint32_t *src, size_t n);

/**
 * @brief Hängt einen 32-Bit-Quellstring an den Zielstring an.
 *
 * @param dest Zielstring.
 * @param src Quellstring.
 * @return Zeiger auf den Zielstring (dest).
 */
uint32_t *u32_strcat(uint32_t *dest, const uint32_t *src);

/**
 * @brief Erstellt eine Duplikatskopie eines 32-Bit-Strings auf dem Heap.
 *
 * @param src Quellstring.
 * @return Zeiger auf den neu allokierten 32-Bit-String oder NULL bei Fehler.
 */
uint32_t *u32_strdup(const uint32_t *src);