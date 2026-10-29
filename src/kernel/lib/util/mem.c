#include <pch.h>
#include <lib/util/mem.h>


void *memset(void *dest, uint8_t value, size_t count) {
    uint64_t val64 = value;
    val64 |= (val64 << 8);
    val64 |= (val64 << 16);
    val64 |= (val64 << 32);

    uint8_t *d = (uint8_t *)dest;
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    if (qwords > 0) {
        ASM_CODE("cld; rep stosq"
                 : "+D"(d), "+c"(qwords)
                 : "a"(val64)
                 : "memory");
    }
    if (bytes > 0) {
        ASM_CODE("rep stosb"
                 : "+D"(d), "+c"(bytes)
                 : "a"(value)
                 : "memory");
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    if (qwords > 0) {
        ASM_CODE("cld; rep movsq"
                 : "+D"(d), "+S"(s), "+c"(qwords)
                 :
                 : "memory");
    }
    if (bytes > 0) {
        ASM_CODE("rep movsb"
                 : "+D"(d), "+S"(s), "+c"(bytes)
                 :
                 : "memory");
    }
    return dest;
}

void memset32(void *dest, uint32_t value, size_t count) {
    ASM_CODE("cld; rep stosl"
             : "+D"(dest), "+c"(count)
             : "a"(value)
             : "memory");
}

void memset64(void *dest, uint64_t value, size_t count) {
    ASM_CODE("cld; rep stosq"
             : "+D"(dest), "+c"(count)
             : "a"(value)
             : "memory");
}

int memcmp(const void *ptr1, const void *ptr2, size_t count) {
    const uint64_t *p1_64 = ptr1;
    const uint64_t *p2_64 = ptr2;
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    for (size_t i = 0; i < qwords; i++) {
        if (p1_64[i] != p2_64[i]) {
            const uint8_t *b1 = (const uint8_t *)&p1_64[i];
            const uint8_t *b2 = (const uint8_t *)&p2_64[i];
            for (size_t j = 0; j < 8; j++) {
                if (b1[j] != b2[j])
                    return (b1[j] < b2[j]) ? -1 : 1;
            }
        }
    }

    const uint8_t *b1 = (const uint8_t *)(p1_64 + qwords);
    const uint8_t *b2 = (const uint8_t *)(p2_64 + qwords);
    for (size_t i = 0; i < bytes; i++) {
        if (b1[i] < b2[i])
            return -1;
        if (b1[i] > b2[i])
            return 1;
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t count) {
    uint8_t *d = dest;
    const uint8_t *s = src;

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
            ASM_CODE("std; rep movsb"
                     : "+D"(d), "+S"(s), "+c"(bytes)
                     :
                     : "memory");
            ASM_CODE("cld" ::: "memory");
        }
        if (qwords > 0) {
            d -= (qwords * 8) - 7;
            s -= (qwords * 8) - 7;
            ASM_CODE("std; rep movsq"
                     : "+D"(d), "+S"(s), "+c"(qwords)
                     :
                     : "memory");
            ASM_CODE("cld" ::: "memory");
        }
    }
    return dest;
}

