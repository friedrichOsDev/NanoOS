#include <string.h>

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

size_t strlen(const char* str) {
    const char* s;
    for (s = str; *s; ++s);
    return (s - str);
}

size_t strnlen(const char* str, size_t maxlen) {
    const char* s;
    for (s = str; *s && maxlen--; ++s);
    return (s - str);
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

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
