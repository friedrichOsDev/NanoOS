/**
 * @file mem.h
 * @brief Memory manipulation functions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

void *memset(void *dest, uint8_t value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void memset32(void *dest, uint32_t value, size_t count);
void memset64(void *dest, uint64_t value, size_t count);
int memcmp(const void *ptr1, const void *ptr2, size_t count);
void *memmove(void *dest, const void *src, size_t count);
