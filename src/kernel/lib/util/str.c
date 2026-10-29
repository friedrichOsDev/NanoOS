/**
 * @file string.c
 * @brief String- und Speicherfunktionen
 * @author friedrichOsDev
 */

#include <lib/util/str.h>
#include <lib/util/mem.h>

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

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

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

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    if (i < n)
        memset(dest + i, 0, n - i);
    return dest;
}

char *strcat(char *dest, const char *src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

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

uint32_t *u32_strcpy(uint32_t *dest, const uint32_t *src) {
    uint32_t *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

int u32_strcmp(const uint32_t *s1, const uint32_t *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*s1 < *s2) ? -1 : (*s1 > *s2);
}

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

uint32_t *u32_strncpy(uint32_t *dest, const uint32_t *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != U'\0'; i++)
        dest[i] = src[i];
    if (i < n)
        memset32(dest + i, 0, n - i);
    return dest;
}

uint32_t *u32_strcat(uint32_t *dest, const uint32_t *src) {
    u32_strcpy(dest + u32_strlen(dest), src);
    return dest;
}
