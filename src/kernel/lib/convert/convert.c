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
static void reverse_str(char* str, int length) {
    for (int i = 0; i < length / 2; ++i) {
        char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
}

/**
 * @brief Converts an unsigned 32-bit integer to a string.
 * 
 * @param value The value to convert.
 * @param buffer The destination buffer.
 * @param base The numerical base (e.g., 10 for decimal, 16 for hex).
 * @return int The length of the resulting string.
 */
int uint_to_str(uint32_t value, char* buffer, int base) {
    int i = 0;

    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return i;
    }

    while (value != 0) {
        uint32_t rem = value % base;
        buffer[i++] = rem > 9 ? (rem - 10) + 'A' : rem + '0';
        value /= base;
    }

    buffer[i] = '\0';
    reverse_str(buffer, i);
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