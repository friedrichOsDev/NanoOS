/**
 * @file string.c
 * @author friedrichOsDev
 */

#include <string.h>

/**
 * @brief Fills a block of memory with a specific byte value.
 * 
 * @param dest Pointer to the memory block to fill.
 * @param value The byte value to set.
 * @param count Number of bytes to fill.
 * @return void* Pointer to the destination memory block.
 */
void* memset(void* dest, uint8_t value, size_t count) {
    __asm__ __volatile__ (
        "cld; rep stosb"
        : "+D"(dest), "+c"(count)
        : "a"(value)
        : "memory"
    );
    return dest;
}

/**
 * @brief Copies a block of memory from source to destination.
 * 
 * @param dest Pointer to the destination memory block.
 * @param src Pointer to the source memory block.
 * @param count Number of bytes to copy.
 * @return void* Pointer to the destination memory block.
 */
void* memcpy(void* dest, const void* src, size_t count) {
    __asm__ __volatile__ (
        "cld; rep movsb"
        : "+D"(dest), "+S"(src), "+c"(count)
        :
        : "memory"
    );
    return dest;
}

/**
 * @brief Compares two blocks of memory.
 * 
 * @param ptr1 Pointer to the first memory block.
 * @param ptr2 Pointer to the second memory block.
 * @param count Number of bytes to compare.
 * @return int 0 if blocks are equal, negative if ptr1 < ptr2, positive if ptr1 > ptr2.
 */
int memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    for (size_t i = 0; i < count; i++) {
        if (p1[i] < p2[i]) {
            return -1;
        } else if (p1[i] > p2[i]) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Calculates the length of a null-terminated uint32_t string.
 * @param str The string.
 * @return The number of characters in the string.
 */
size_t u32_strlen(const uint32_t* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/**
 * @brief Copies a null-terminated uint32_t string.
 * @param dest The destination buffer.
 * @param src The source string.
 * @return A pointer to the destination buffer.
 */
uint32_t* u32_strcpy(uint32_t* dest, const uint32_t* src) {
    uint32_t* d = dest;
    while ((*d++ = *src++));
    return dest;
}

/**
 * @brief Compares two null-terminated uint32_t strings.
 * @param s1 The first string.
 * @param s2 The second string.
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2.
 */
int u32_strcmp(const uint32_t* s1, const uint32_t* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const uint32_t*)s1 - *(const uint32_t*)s2;
}