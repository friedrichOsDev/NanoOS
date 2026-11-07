/**
 * @file io.c
 * @brief I/O port operations.
 * @author friedrichOsDev
 */

#include <arch/arch.h>

/**
 * @brief Write a byte to an I/O port.
 *
 * @param port The I/O port to write to.
 * @param val The byte to write.
 */
void outb(uint16_t port, uint8_t val) {
    ASM_CODE("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Read a byte from an I/O port.
 *
 * @param port The I/O port to read from.
 * @return The byte read from the I/O port.
 */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    ASM_CODE("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * @brief Write a word to an I/O port.
 *
 * @param port The I/O port to write to.
 * @param val The word to write.
 */
void outw(uint16_t port, uint16_t val) {
    ASM_CODE("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Read a word from an I/O port.
 *
 * @param port The I/O port to read from.
 * @return The word read from the I/O port.
 */
uint16_t inw(uint16_t port) {
    uint16_t ret;
    ASM_CODE("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * @brief Write a long to an I/O port.
 *
 * @param port The I/O port to write to.
 * @param val The long to write.
 */
void outl(uint16_t port, uint32_t val) {
    ASM_CODE("outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Read a long from an I/O port.
 *
 * @param port The I/O port to read from.
 * @return The long read from the I/O port.
 */
uint32_t inl(uint16_t port) {
    uint32_t ret;
    ASM_CODE("inl %w1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}
