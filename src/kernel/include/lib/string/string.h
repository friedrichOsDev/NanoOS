/**
 * @file string.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

void* memset(void* dest, uint8_t value, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
void memset32(void* dest, uint32_t value, size_t count);
void memcpy32(void* dest, const void* src, size_t count);
int memcmp(const void* ptr1, const void* ptr2, size_t count);
void* memmove(void* dest, const void* src, size_t count);

size_t u32_strlen(const uint32_t* str);
uint32_t* u32_strcpy(uint32_t* dest, const uint32_t* src);
int u32_strcmp(const uint32_t* s1, const uint32_t* s2);
int u32_strncmp(const uint32_t* s1, const uint32_t* s2, size_t n);
uint32_t* u32_strncpy(uint32_t* dest, const uint32_t* src, size_t n);
uint32_t* u32_strcat(uint32_t* dest, const uint32_t* src);
uint32_t* u32_strdup(const uint32_t* src);
