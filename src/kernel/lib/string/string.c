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