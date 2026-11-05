/**
 * @file str.h
 * @brief String manipulation functions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strdup(const char *src);

size_t u32_strlen(const uint32_t *str);
uint32_t *u32_strcpy(uint32_t *dest, const uint32_t *src);
int u32_strcmp(const uint32_t *s1, const uint32_t *s2);
int u32_strncmp(const uint32_t *s1, const uint32_t *s2, size_t n);
uint32_t *u32_strncpy(uint32_t *dest, const uint32_t *src, size_t n);
uint32_t *u32_strcat(uint32_t *dest, const uint32_t *src);
uint32_t *u32_strdup(const uint32_t *src);
