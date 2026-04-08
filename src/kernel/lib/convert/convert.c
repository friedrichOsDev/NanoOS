/**
 * @file convert.c
 * @author friedrichOsDev
 */

#include <convert.h>
#include <serial.h>

/**
 * @brief Reverses a string in place.
 * 
 * @param str The string to reverse.
 * @param length The length of the string.
 */
static void reverse_str(uint32_t* str, int length) {
    for (int i = 0; i < length / 2; ++i) {
        uint32_t temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = (uint32_t)temp;
    }
}

/**
 * @brief Reverses a string in place (legacy version).
 * 
 * @param str The string to reverse.
 * @param length The length of the string.
 */
static void reverse_str_legacy(char* str, int length) {
    for (int i = 0; i < length / 2; ++i) {
        char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
}

/**
 * @brief Teilt eine 64-Bit Zahl durch eine 32-Bit Zahl und gibt den Rest zurück.
 * Dies verhindert, dass der Compiler nach __udivdi3 sucht.
 * @param dividend Pointer auf die 64-Bit Zahl (wird mit dem Quotienten aktualisiert).
 * @param divisor Die 32-Bit Basis (Divisor).
 * @return uint32_t Der Rest der Division.
 */
static uint32_t div64_32(uint64_t* dividend, uint32_t divisor) {
    uint32_t high = (uint32_t)(*dividend >> 32);
    uint32_t low = (uint32_t)(*dividend & 0xFFFFFFFF);
    uint32_t rem;
    uint32_t high_q, low_q;

    __asm__ __volatile__ ("divl %4" : "=a"(high_q), "=d"(rem) : "a"(high), "d"(0), "r"(divisor));
    __asm__ __volatile__ ("divl %4" : "=a"(low_q), "=d"(rem) : "a"(low), "d"(rem), "r"(divisor));

    *dividend = ((uint64_t)high_q << 32) | low_q;
    return rem;
}

/**
 * @brief Converts an unsigned 32-bit integer to a string.
 * 
 * @param value The value to convert.
 * @param buffer The destination buffer.
 * @param base The numerical base (e.g., 10 for decimal, 16 for hex).
 * @return int The length of the resulting string.
 */
int uint_to_str(uint64_t value, uint32_t* buffer, int base) {
    int i = 0;

    if (value == 0) {
        buffer[i++] = U'0';
        buffer[i] = U'\0';
        return i;
    }

    while (value != 0) {
        uint32_t rem = div64_32(&value, (uint32_t)base);
        buffer[i++] = rem > 9 ? (rem - 10) + U'A' : rem + U'0';
    }

    buffer[i] = U'\0';
    reverse_str(buffer, i);
    return i;
}

/**
 * @brief Converts an unsigned 32-bit integer to a string (legacy version).
 * 
 * @param value The value to convert.
 * @param buffer The destination buffer.
 * @param base The numerical base (e.g., 10 for decimal, 16 for hex).
 * @return int The length of the resulting string.
 */
int uint_to_str_legacy(uint64_t value, char* buffer, int base) {
    int i = 0;

    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return i;
    }

    while (value != 0) {
        uint32_t rem = div64_32(&value, (uint32_t)base);
        buffer[i++] = rem > 9 ? (rem - 10) + 'A' : rem + '0';
    }

    buffer[i] = '\0';
    reverse_str_legacy(buffer, i);
    return i;
}

/**
 * @brief Converts a BCD (Binary Coded Decimal) byte to a decimal byte.
 * 
 * @param bcd The BCD encoded value.
 * @return uint8_t The decimal representation.
 */
uint8_t bcd_to_dezimal(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/**
 * @brief Converts a null-terminated uint32_t string to a char string.
 * @param ustr The source uint32_t string.
 * @param buffer The destination char buffer.
 * @param buffer_size The size of the destination buffer.
 * @return A pointer to the destination buffer.
 */
char * ustr_to_str(const uint32_t* ustr, char* buffer, size_t buffer_size) {
    size_t i;
    for (i = 0; i < buffer_size - 1 && ustr[i] != U'\0'; i++) {
        buffer[i] = (char)ustr[i];
    }
    buffer[i] = '\0';
    return buffer;
}
