/**
 * @file string.c
 * @brief String lib
 * @author friedrichOsDev
 */

#include <lib/string.h>

/**
 * Fills a block of memory with a specific 8-bit value
 * @param dest Pointer to the block of memory to fill
 * @param value The value to be set
 * @param count Number of bytes to fill
 * @return A pointer to the destination memory area (dest)
 */
void* memset(void* dest, uint8_t value, size_t count) {
    uint64_t val64 = value;
    val64 |= (val64 << 8);
    val64 |= (val64 << 16);
    val64 |= (val64 << 32);

    uint8_t* d = (uint8_t*)dest;
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    if (qwords > 0) {
        __asm__ __volatile__ (
            "cld; rep stosq"
            : "+D"(d), "+c"(qwords)
            : "a"(val64)
            : "memory"
        );
    }
    if (bytes > 0) {
        __asm__ __volatile__ (
            "rep stosb"
            : "+D"(d), "+c"(bytes)
            : "a"(value)
            : "memory"
        );
    }
    return dest;
}

/**
 * Copies count bytes from source to destination memory area
 * @param dest Pointer to the destination array
 * @param src Pointer to the source of data to be copied
 * @param count Number of bytes to copy
 * @return A pointer to the destination memory area (dest)
 */
void* memcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    if (qwords > 0) {
        __asm__ __volatile__ (
            "cld; rep movsq"
            : "+D"(d), "+S"(s), "+c"(qwords)
            :
            : "memory"
        );
    }
    if (bytes > 0) {
        __asm__ __volatile__ (
            "rep movsb"
            : "+D"(d), "+S"(s), "+c"(bytes)
            :
            : "memory"
        );
    }
    return dest;
}

/**
 * Fills a block of memory with a specific 32-bit value
 * @param dest Pointer to the block of memory to fill
 * @param value The 32-bit value to be set
 * @param count Number of 32-bit dwords to fill
 */
void memset32(void* dest, uint32_t value, size_t count) {
    __asm__ __volatile__(
        "cld; rep stosl"
        : "+D" (dest), "+c" (count)
        : "a" (value)
        : "memory"
    );
}

/**
 * Copies count 32-bit dwords from source to destination memory area
 * @param dest Pointer to the destination array
 * @param src Pointer to the source of data to be copied
 * @param count Number of 32-bit dwords to copy
 */
void memcpy32(void* dest, const void* src, size_t count) {
    __asm__ __volatile__(
        "cld; rep movsl"
        : "+D" (dest), "+S" (src), "+c" (count)
        :
        : "memory"
    );
}

/**
 * Fills a block of memory with a specific 64-bit value
 * @param dest Pointer to the block of memory to fill
 * @param value The 64-bit value to be set
 * @param count Number of 64-bit qwords to fill
 */
void memset64(void* dest, uint64_t value, size_t count) {
    __asm__ __volatile__(
        "cld; rep stosq"
        : "+D" (dest), "+c" (count)
        : "a" (value)
        : "memory"
    );
}

/**
 * Copies count 64-bit qwords from source to destination memory area
 * @param dest Pointer to the destination array
 * @param src Pointer to the source of data to be copied
 * @param count Number of 64-bit qwords to copy
 */
void memcpy64(void* dest, const void* src, size_t count) {
    __asm__ __volatile__(
        "cld; rep movsq"
        : "+D" (dest), "+S" (src), "+c" (count)
        :
        : "memory"
    );
}

/**
 * Compares two blocks of memory
 * @param ptr1 Pointer to the first block of memory
 * @param ptr2 Pointer to the second block of memory
 * @param count Number of bytes to compare
 * @return 0 if matches, < 0 if ptr1 is less than ptr2, > 0 if ptr1 is greater
 */
int memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const uint64_t* p1_64 = ptr1;
    const uint64_t* p2_64 = ptr2;
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    for (size_t i = 0; i < qwords; i++) {
        if (p1_64[i] != p2_64[i]) {
            const uint8_t* b1 = (const uint8_t*)&p1_64[i];
            const uint8_t* b2 = (const uint8_t*)&p2_64[i];
            for (size_t j = 0; j < 8; j++) {
                if (b1[j] != b2[j]) return (b1[j] < b2[j]) ? -1 : 1;
            }
        }
    }

    const uint8_t* b1 = (const uint8_t*)(p1_64 + qwords);
    const uint8_t* b2 = (const uint8_t*)(p2_64 + qwords);
    for (size_t i = 0; i < bytes; i++) {
        if (b1[i] < b2[i]) return -1;
        if (b1[i] > b2[i]) return 1;
    }
    return 0;
}

/**
 * Copies a block of memory, handling overlapping regions safely
 * @param dest Pointer to the destination array
 * @param src Pointer to the source of data to be copied
 * @param count Number of bytes to copy
 * @return A pointer to the destination memory area (dest)
 */
void* memmove(void* dest, const void* src, size_t count) {
    uint8_t* d = dest;
    const uint8_t* s = src;

    if (d < s) {
        return memcpy(dest, src, count);
    } else if (d > s) {
        d += count;
        s += count;

        size_t qwords = count / 8;
        size_t bytes = count % 8;

        if (bytes > 0) {
            d -= bytes;
            s -= bytes;
            __asm__ __volatile__ (
                "std; rep movsb"
                : "+D"(d), "+S"(s), "+c"(bytes)
                :
                : "memory"
            );
            __asm__ __volatile__ ("cld" ::: "memory");
        }
        if (qwords > 0) {
            d -= (qwords * 8) - 7;
            s -= (qwords * 8) - 7;
            __asm__ __volatile__ (
                "std; rep movsq"
                : "+D"(d), "+S"(s), "+c"(qwords)
                :
                : "memory"
            );
            __asm__ __volatile__ ("cld" ::: "memory");
        }
    }
    return dest;
}

/**
 * Computes the length of a null-terminated string using optimized word-access
 * @param str Pointer to the null-terminated string
 * @return The number of characters in the string before the terminating null byte
 */
size_t strlen(const char* str) {
    const char* char_ptr = str;
    const uint64_t* longword_ptr;
    uint64_t longword, magic_bits, hole_bits;

    for (; ((uintptr_t)char_ptr & 7) != 0; ++char_ptr) {
        if (*char_ptr == '\0') return char_ptr - str;
    }

    longword_ptr = (const uint64_t*)char_ptr;
    magic_bits = 0x0101010101010101ULL;
    hole_bits  = 0x8080808080808080ULL;

    for (;;) {
        longword = *longword_ptr++;
        if (((longword - magic_bits) & ~longword & hole_bits) != 0) {
            const char* cp = (const char*)(longword_ptr - 1);
            if (cp[0] == 0) return cp - str;
            if (cp[1] == 0) return cp - str + 1;
            if (cp[2] == 0) return cp - str + 2;
            if (cp[3] == 0) return cp - str + 3;
            if (cp[4] == 0) return cp - str + 4;
            if (cp[5] == 0) return cp - str + 5;
            if (cp[6] == 0) return cp - str + 6;
            if (cp[7] == 0) return cp - str + 7;
        }
    }
}

/**
 * Copies the null-terminated string from source to destination
 * @param dest Pointer to the destination array
 * @param src Pointer to the null-terminated string to copy
 * @return A pointer to the destination string (dest)
 */
char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

/**
 * Compares two null-terminated strings lexicographically
 * @param s1 Pointer to the first string
 * @param s2 Pointer to the second string
 * @return An integer <, ==, or > 0 if s1 is found to be less, matching, or greater than s2
 */
int strcmp(const char* s1, const char* s2) {
    while (((uintptr_t)s1 & 7) != 0 || ((uintptr_t)s2 & 7) != 0) {
        if (*s1 != *s2) return *(const unsigned char*)s1 - *(const unsigned char*)s2;
        if (*s1 == '\0') return 0;
        s1++; s2++;
    }

    const uint64_t* l1 = (const uint64_t*)s1;
    const uint64_t* l2 = (const uint64_t*)s2;
    uint64_t magic_bits = 0x0101010101010101ULL;
    uint64_t hole_bits  = 0x8080808080808080ULL;

    while (*l1 == *l2) {
        if (((*l1 - magic_bits) & ~*l1 & hole_bits) != 0) break;
        l1++; l2++;
    }

    s1 = (const char*)l1;
    s2 = (const char*)l2;
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * Compares up to n characters of two null-terminated strings
 * @param s1 Pointer to the first string
 * @param s2 Pointer to the second string
 * @param n Maximum number of characters to compare
 * @return An integer <, ==, or > 0 if s1 is found to be less, matching, or greater than s2
 */
int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * Copies up to n characters from source to destination, padding with null bytes if needed
 * @param dest Pointer to the destination array
 * @param src Pointer to the null-terminated string to copy
 * @param n Maximum number of characters to copy
 * @return A pointer to the destination string (dest)
 */
char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    if (i < n) memset(dest + i, 0, n - i);
    return dest;
}

/**
 * Appends the source string to the destination string
 * @param dest Pointer to the destination string (must contain null-terminated string and have enough space)
 * @param src Pointer to the null-terminated string to append
 * @return A pointer to the destination string (dest)
 */
char* strcat(char* dest, const char* src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

/**
 * Computes the length of a null-terminated 32-bit wide character string
 * @param str Pointer to the null-terminated 32-bit wide string
 * @return The number of 32-bit characters in the string before the terminating null
 */
size_t u32_strlen(const uint32_t* str) {
    const uint32_t* s = str;
    while (((uintptr_t)s & 7) != 0) {
        if (*s == 0) return s - str;
        s++;
    }
    const uint64_t* l = (const uint64_t*)s;
    for (;;) {
        uint64_t val = *l;
        if ((val & 0xFFFFFFFF) == 0) return (const uint32_t*)l - str;
        if ((val >> 32) == 0) return (const uint32_t*)l - str + 1;
        l++;
    }
}

/**
 * Copies the null-terminated 32-bit wide string from source to destination
 * @param dest Pointer to the destination 32-bit array
 * @param src Pointer to the null-terminated 32-bit wide string to copy
 * @return A pointer to the destination string (dest)
 */
uint32_t* u32_strcpy(uint32_t* dest, const uint32_t* src) {
    uint32_t* d = dest;
    while ((*d++ = *src++));
    return dest;
}

/**
 * Compares two null-terminated 32-bit wide strings lexicographically
 * @param s1 Pointer to the first 32-bit string
 * @param s2 Pointer to the second 32-bit string
 * @return -1 if s1 < s2, 1 if s1 > s2, or 0 if they match
 */
int u32_strcmp(const uint32_t* s1, const uint32_t* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return (*s1 < *s2) ? -1 : (*s1 > *s2);
}

/**
 * Compares up to n characters of two null-terminated 32-bit wide strings
 * @param s1 Pointer to the first 32-bit string
 * @param s2 Pointer to the second 32-bit string
 * @param n Maximum number of characters to compare
 * @return -1 if s1 < s2, 1 if s1 > s2, or 0 if they match
 */
int u32_strncmp(const uint32_t* s1, const uint32_t* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return (*s1 < *s2) ? -1 : (*s1 > *s2);
}

/**
 * Copies up to n characters from source to destination 32-bit array, padding with zeros if needed
 * @param dest Pointer to the destination 32-bit array
 * @param src Pointer to the null-terminated 32-bit wide string to copy
 * @param n Maximum number of 32-bit characters to copy
 * @return A pointer to the destination string (dest)
 */
uint32_t* u32_strncpy(uint32_t* dest, const uint32_t* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != U'\0'; i++) dest[i] = src[i];
    if (i < n) memset32(dest + i, 0, n - i);
    return dest;
}

/**
 * Appends the source 32-bit wide string to the destination 32-bit wide string
 * @param dest Pointer to the destination 32-bit array (must contain null-terminated string and have enough space)
 * @param src Pointer to the null-terminated 32-bit wide string to append
 * @return A pointer to the destination string (dest)
 */
uint32_t* u32_strcat(uint32_t* dest, const uint32_t* src) {
    u32_strcpy(dest + u32_strlen(dest), src);
    return dest;
}