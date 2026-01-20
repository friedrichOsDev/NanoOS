/*
 * @file string.h
 * @brief Header file for string manipulation functions
 * @author friedrichOsDev
 */

#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

int strcmp(const char* s1, const char* s2);
char* strcpy(char* dest, const char* src);
size_t strlen(const char* str);
size_t strnlen(const char* str, size_t maxlen);
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

#endif // STRING_H
