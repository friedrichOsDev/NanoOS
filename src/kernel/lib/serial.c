/*
 * @file serial.c
 * @brief Serial driver implementation
 * @author friedrichOsDev
 */

#include <serial.h>
#include <io.h>

/*
 * A function to initialize the serial port (COM1)
 * @param void
 */
void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);   
    outb(COM1_PORT + 3, 0x80);   
    outb(COM1_PORT + 0, 0x03);   
    outb(COM1_PORT + 1, 0x00);   
    outb(COM1_PORT + 3, 0x03);   
    outb(COM1_PORT + 2, 0xC7);   
    outb(COM1_PORT + 4, 0x0B);   
}

/*
 * Checks if the transmit buffer is empty
 * @param void
 * @return True if the transmit buffer is empty, false otherwise
 */
static bool serial_is_transmit_empty(void) {
    return (inb(COM1_PORT + 5) & 0x20) != 0;
}

/*
 * Writes a character to the serial port
 * @param c The character to write
 * @note This function uses the "no-sse" target attribute to avoid SSE instructions (mabey it's unnecessary, but for now I keep it)
 */
void __attribute__((__target__("no-sse"))) serial_putc(char c) {
    while (!serial_is_transmit_empty()); 
    outb(COM1_PORT, c);
}

/*
 * Writes a null-terminated string to the serial port
 * @param s The string to write
 * @note This function uses the "no-sse" target attribute to avoid SSE instructions (mabey it's unnecessary, but for now I keep it)
 */
void __attribute__((__target__("no-sse"))) serial_puts(const char* s) {
    while (*s) {
        serial_putc(*s++);
    }
}

/*
 * Writes a hexadecimal representation of a 32-bit value to the serial port
 * @param value The value to write
 * @note This function uses the "no-sse" target attribute to avoid SSE instructions (mabey it's unnecessary, but for now I keep it)
 */
void __attribute__((__target__("no-sse"))) serial_put_hex(uint32_t value) {
    char hex_digits[] = "0123456789abcdef";
    char buffer[9]; 
    int i = 7;

    buffer[8] = '\0';

    if (value == 0) {
        serial_putc('0');
        return;
    }

    while (value > 0 && i >= 0) {
        buffer[i--] = hex_digits[value % 16];
        value /= 16;
    }

    while (i >= 0) { 
        buffer[i--] = '0';
    }

    serial_puts(buffer);
}

/*
 * Writes a decimal representation of a 32-bit value to the serial port
 * @param value The value to write
 * @note This function uses the "no-sse" target attribute to avoid SSE instructions (mabey it's unnecessary, but for now I keep it)
 */
void __attribute__((__target__("no-sse"))) serial_put_int(uint32_t value) {
    char buffer[12]; 
    int i = 0;

    if (value == 0) {
        serial_putc('0');
        return;
    }

    while (value != 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        serial_putc(buffer[j]);
    }
}
