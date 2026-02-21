/*
 * @file string.c
 * @brief Basic string and memory manipulation functions
 * @author friedrichOsDev
 */

#include <string.h>

/*
 * Set a block of memory to a specific value
 * @param dest The destination address to set
 * @param value The value to set each byte to
 * @param count The number of bytes to set
 * @return The original destination pointer
 */
void* memset(void* dest, uint8_t value, size_t count) {
    uint8_t* ptr = (uint8_t*)dest;
    while (count--) {
        *ptr++ = value;
    }
    return dest;
}

/*
 * Copy a block of memory from one location to another
 * @param dest The destination address to copy to
 * @param src The source address to copy from
 * @param count The number of bytes to copy
 * @return The original destination pointer
 */
void* memcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}