/**
 * @file string.c
 * @brief String lib
 * @author friedrichOsDev
 */

#include <lib/string.h>

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

void memset32(void* dest, uint32_t value, size_t count) {
    __asm__ __volatile__(
        "cld; rep stosl"
        : "+D" (dest), "+c" (count)
        : "a" (value)
        : "memory"
    );
}

void memcpy32(void* dest, const void* src, size_t count) {
    __asm__ __volatile__(
        "cld; rep movsl"
        : "+D" (dest), "+S" (src), "+c" (count)
        :
        : "memory"
    );
}

void memset64(void* dest, uint64_t value, size_t count) {
    __asm__ __volatile__(
        "cld; rep stosq"
        : "+D" (dest), "+c" (count)
        : "a" (value)
        : "memory"
    );
}

void memcpy64(void* dest, const void* src, size_t count) {
    __asm__ __volatile__(
        "cld; rep movsq"
        : "+D" (dest), "+S" (src), "+c" (count)
        :
        : "memory"
    );
}

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

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

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

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    if (i < n) memset(dest + i, 0, n - i);
    return dest;
}

char* strcat(char* dest, const char* src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

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

uint32_t* u32_strcpy(uint32_t* dest, const uint32_t* src) {
    uint32_t* d = dest;
    while ((*d++ = *src++));
    return dest;
}

int u32_strcmp(const uint32_t* s1, const uint32_t* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return (*s1 < *s2) ? -1 : (*s1 > *s2);
}

int u32_strncmp(const uint32_t* s1, const uint32_t* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return (*s1 < *s2) ? -1 : (*s1 > *s2);
}

uint32_t* u32_strncpy(uint32_t* dest, const uint32_t* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != U'\0'; i++) dest[i] = src[i];
    if (i < n) memset32(dest + i, 0, n - i);
    return dest;
}

uint32_t* u32_strcat(uint32_t* dest, const uint32_t* src) {
    u32_strcpy(dest + u32_strlen(dest), src);
    return dest;
}