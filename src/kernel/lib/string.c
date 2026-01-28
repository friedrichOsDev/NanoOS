/*
 * @file string.c
 * @brief String manipulation functions
 * @author friedrichOsDev
 */

#include <string.h>

/*
 * A strcmp implementation
 * @param s1 The first string
 * @param s2 The second string
 * @return An integer (interpreted as boolean) indicating the result of the comparison
 */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/*
 * A strcpy implementation
 * @param dest The destination string
 * @param src The source string
 * @return The destination string
 */
char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

/*
 * A strlen implementation
 * @param str The string to measure
 * @return The length of the string
 */
size_t strlen(const char* str) {
    const char* s;
    for (s = str; *s; ++s);
    return (s - str);
}

/*
 * A strnlen implementation
 * @param str The string to measure
 * @param maxlen The maximum number of characters to examine
 * @return The length of the string, or maxlen if the string is longer
 */
size_t strnlen(const char* str, size_t maxlen) {
    const char* s;
    for (s = str; *s && maxlen--; ++s);
    return (s - str);
}

/*
 * A memset implementation
 * @param ptr The pointer to the memory area
 * @param value The value to set
 * @param num The number of bytes to set
 * @return The pointer to the memory area
 */
void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

/*
 * A memcpy implementation
 * @param dest The destination memory area
 * @param src The source memory area
 * @param n The number of bytes to copy
 * @return The destination memory area
 */
void* memcpy(void* dest, const void* src, size_t n) {
    uint32_t* d32 = (uint32_t*)dest;
    const uint32_t* s32 = (const uint32_t*)src;

    // 4-bytes per copy
    size_t words = n / 4;
    for (size_t i = 0; i < words; i++) {
        d32[i] = s32[i];
    }

    // n not multiple of 4, copy remaining bytes
    size_t rest = n % 4;
    if (rest > 0) {
        uint8_t* d8 = (uint8_t*)(d32 + words);
        const uint8_t* s8 = (const uint8_t*)(s32 + words);
        for (size_t i = 0; i < rest; i++) {
            d8[i] = s8[i];
        }
    }
    return dest;
}

/*
 * A memcmp implementation
 * @param ptr1 The first memory area
 * @param ptr2 The second memory area
 * @param num The number of bytes to compare
 * @return An integer (interpreted as boolean) indicating the result of the comparison
 */
int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = ptr1;
    const unsigned char* p2 = ptr2;
    while (num--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

/*
 * TODO: replace all manual implementations with standard library versions of atoi and itoa
 */

/*
 * A atoi implementation
 * @param str The string to convert
 * @return The converted integer value
 */
int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;
    if (str[0] == '-') {
        sign = -1;
        i++;
    }
    for (; str[i] != '\0'; ++i) {
        res = res * 10 + str[i] - '0';
    }
    return sign * res;
}

/*
 * An itoa implementation
 * @param value The integer value to convert
 * @param str The string to store the result
 * @param base The numerical base for conversion
 * @return The converted string
 */
char* itoa(int value, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_value;

    if (value < 0 && base == 10) {
        value = -value;
        *ptr++ = '-';
        ptr1++;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789ABCDEF"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}
