/**
 * @file str.c
 * @brief String manipulation functions implementation.
 * @author friedrichOsDev
 */

#include <lib/util/mem.h>
#include <lib/util/str.h>

/**
 * @brief Calculates the length of a null-terminated ASCII string.
 *
 * @param[in] str Pointer to the null-terminated string.
 * @return Length of the string in bytes (excluding null-terminator).
 */
size_t strlen(const char *str) {
    const char *char_ptr = str;
    const uint64_t *longword_ptr;
    uint64_t longword, magic_bits, hole_bits;

    for (; ((uintptr_t)char_ptr & 7) != 0; ++char_ptr) {
        if (*char_ptr == '\0')
            return (size_t)(char_ptr - str);
    }

    longword_ptr = (const uint64_t *)char_ptr;
    magic_bits = 0x0101010101010101ULL;
    hole_bits = 0x8080808080808080ULL;

    for (;;) {
        longword = *longword_ptr++;
        if (((longword - magic_bits) & ~longword & hole_bits) != 0) {
            const char *cp = (const char *)(longword_ptr - 1);
            if (cp[0] == 0)
                return (size_t)(cp - str);
            if (cp[1] == 0)
                return (size_t)(cp - str + 1);
            if (cp[2] == 0)
                return (size_t)(cp - str + 2);
            if (cp[3] == 0)
                return (size_t)(cp - str + 3);
            if (cp[4] == 0)
                return (size_t)(cp - str + 4);
            if (cp[5] == 0)
                return (size_t)(cp - str + 5);
            if (cp[6] == 0)
                return (size_t)(cp - str + 6);
            if (cp[7] == 0)
                return (size_t)(cp - str + 7);
        }
    }
}

/**
 * @brief Copies a null-terminated ASCII string to a destination buffer.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source string.
 * @return Pointer to the destination buffer.
 */
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

/**
 * @brief Compares two null-terminated ASCII strings.
 *
 * @param[in] s1 Pointer to the first string.
 * @param[in] s2 Pointer to the second string.
 * @return Returns 0 if equal, < 0 if s1 < s2, > 0 if s1 > s2.
 */
int strcmp(const char *s1, const char *s2) {
    while (((uintptr_t)s1 & 7) != 0 || ((uintptr_t)s2 & 7) != 0) {
        if (*s1 != *s2)
            return *(const unsigned char *)s1 - *(const unsigned char *)s2;
        if (*s1 == '\0')
            return 0;
        s1++;
        s2++;
    }

    const uint64_t *l1 = (const uint64_t *)s1;
    const uint64_t *l2 = (const uint64_t *)s2;
    uint64_t magic_bits = 0x0101010101010101ULL;
    uint64_t hole_bits = 0x8080808080808080ULL;

    while (*l1 == *l2) {
        if (((*l1 - magic_bits) & ~*l1 & hole_bits) != 0)
            break;
        l1++;
        l2++;
    }

    s1 = (const char *)l1;
    s2 = (const char *)l2;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * @brief Compares up to n characters of two ASCII strings.
 *
 * @param[in] s1 Pointer to the first string.
 * @param[in] s2 Pointer to the second string.
 * @param[in] n Maximum number of characters to compare.
 * @return Returns 0 if equal, < 0 if s1 < s2, > 0 if s1 > s2.
 */
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
        return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * @brief Copies up to n characters of an ASCII string to a destination buffer.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source string.
 * @param[in] n Maximum number of characters to copy.
 * @return Pointer to the destination buffer.
 */
char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    if (i < n)
        memset(dest + i, 0, n - i);
    return dest;
}

/**
 * @brief Appends a source ASCII string to the end of a destination string.
 *
 * @param[out] dest Pointer to the destination string buffer.
 * @param[in] src Pointer to the source string to append.
 * @return Pointer to the destination buffer.
 */
char *strcat(char *dest, const char *src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

/**
 * @brief Calculates the length of a null-terminated UTF-32 wide string.
 *
 * @param[in] str Pointer to the wide string.
 * @return Length of the wide string in characters.
 */
size_t u32_strlen(const uint32_t *str) {
    const uint32_t *s = str;
    while (((uintptr_t)s & 7) != 0) {
        if (*s == 0)
            return (size_t)(s - str);
        s++;
    }
    const uint64_t *l = (const uint64_t *)s;
    for (;;) {
        uint64_t val = *l;
        if ((val & 0xFFFFFFFF) == 0)
            return (size_t)((const uint32_t *)l - str);
        if ((val >> 32) == 0)
            return (size_t)((const uint32_t *)l - str + 1);
        l++;
    }
}

/**
 * @brief Copies a null-terminated UTF-32 wide string.
 *
 * @param[out] dest Pointer to the destination wide string buffer.
 * @param[in] src Pointer to the source wide string.
 * @return Pointer to the destination buffer.
 */
uint32_t *u32_strcpy(uint32_t *dest, const uint32_t *src) {
    uint32_t *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

/**
 * @brief Compares two null-terminated UTF-32 wide strings.
 *
 * @param[in] s1 Pointer to the first wide string.
 * @param[in] s2 Pointer to the second wide string.
 * @return Returns 0 if equal, < 0 if s1 < s2, > 0 if s1 > s2.
 */
int u32_strcmp(const uint32_t *s1, const uint32_t *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*s1 < *s2) ? -1 : (*s1 > *s2);
}

/**
 * @brief Compares up to n characters of two UTF-32 wide strings.
 *
 * @param[in] s1 Pointer to the first wide string.
 * @param[in] s2 Pointer to the second wide string.
 * @param[in] n Maximum number of wide characters to compare.
 * @return Returns 0 if equal, < 0 if s1 < s2, > 0 if s1 > s2.
 */
int u32_strncmp(const uint32_t *s1, const uint32_t *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
        return 0;
    return (*s1 < *s2) ? -1 : (*s1 > *s2);
}

/**
 * @brief Copies up to n characters of a UTF-32 wide string.
 *
 * @param[out] dest Pointer to the destination wide string buffer.
 * @param[in] src Pointer to the source wide string.
 * @param[in] n Maximum number of wide characters to copy.
 * @return Pointer to the destination buffer.
 */
uint32_t *u32_strncpy(uint32_t *dest, const uint32_t *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != U'\0'; i++)
        dest[i] = src[i];
    if (i < n)
        memset32(dest + i, 0, n - i);
    return dest;
}

/**
 * @brief Appends a source UTF-32 wide string to the end of a destination wide string.
 *
 * @param[out] dest Pointer to the destination wide string buffer.
 * @param[in] src Pointer to the source wide string to append.
 * @return Pointer to the destination buffer.
 */
uint32_t *u32_strcat(uint32_t *dest, const uint32_t *src) {
    u32_strcpy(dest + u32_strlen(dest), src);
    return dest;
}
