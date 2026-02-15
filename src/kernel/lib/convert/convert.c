/*
 * @file convert.c
 * @brief Type conversion functions
 * @author friedrichOsDev
 */

#include <convert.h>

/*
 * Convert a hexadecimal value to a string
 * @param value The hexadecimal value to convert
 * @return A string representation of the hexadecimal value
 */
const char* hex_to_str(uint32_t value) {
    static char buffer[11];
    const char* hex_digits = "0123456789ABCDEF";
    
    buffer[0] = '0';
    buffer[1] = 'x';
    
    for (int i = 0; i < 8; i++) {
        buffer[2 + i] = hex_digits[(value >> (28 - i * 4)) & 0xF];
    }
    
    buffer[10] = '\0';
    return buffer;
}

/*
 * Convert an integer value to a string
 * @param value The integer value to convert
 * @return A string representation of the integer value
 */
const char* int_to_str(int32_t value) {
    static char buffer[12];
    int i = 10;
    int negative = 0;

    buffer[11] = '\0';

    if (value == 0) {
        buffer[10] = '0';
        return &buffer[10];
    }

    if (value < 0) {
        negative = 1;
        value = -value;
    }

    while (value > 0 && i >= 0) {
        buffer[i--] = '0' + (value % 10);
        value /= 10;
    }

    if (negative) {
        buffer[i--] = '-';
    }

    return &buffer[i + 1];
}

/*
 * Convert a string to an integer
 * @param str The string to convert
 * @return The integer value of the string
 */
int32_t str_to_int(const char* str) {
    int32_t res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; ++i) {
        if (str[i] < '0' || str[i] > '9') break;
        res = res * 10 + str[i] - '0';
    }

    return sign * res;
}

/*
 * Convert a string to a hexadecimal value
 * @param str The string to convert
 * @return The hexadecimal value of the string
 */
uint32_t str_to_hex(const char* str) {
    uint32_t res = 0;
    int i = 0;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        i = 2;
    }

    for (; str[i] != '\0'; ++i) {
        uint8_t byte = str[i];
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F') byte = byte - 'A' + 10;
        else break;

        res = (res << 4) | (byte & 0xF);
    }

    return res;
}