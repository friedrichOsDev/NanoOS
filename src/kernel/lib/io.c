/**
 * @file io.c
 * @brief 64-Bit Port I/O implementation
 * @author friedrichOsDev
 */

#include <lib/io.h>

/**
 * Sends an 8-bit byte to the specified I/O port
 * @param port The 16-bit I/O port address to write to
 * @param val The 8-bit value to send
 */
void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * Reads an 8-bit byte from the specified I/O port
 * @param port The 16-bit I/O port address to read from
 * @return The 8-bit value read from the port
 */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * Sends a 16-bit word to the specified I/O port
 * @param port The 16-bit I/O port address to write to
 * @param val The 16-bit value to send
 */
void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * Reads a 16-bit word from the specified I/O port
 * @param port The 16-bit I/O port address to read from
 * @return The 16-bit value read from the port
 */
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * Sends a 32-bit double-word to the specified I/O port
 * @param port The 16-bit I/O port address to write to
 * @param val The 32-bit value to send
 */
void outl(uint16_t port, uint32_t val) {
    __asm__ __volatile__("outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * Reads a 32-bit double-word from the specified I/O port
 * @param port The 16-bit I/O port address to read from
 * @return The 32-bit value read from the port
 */
uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__("inl %w1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * Forces a tiny delay to allow hardware-side I/O operations to catch up
 */
void io_wait(void) { outb(0x80, 0); }