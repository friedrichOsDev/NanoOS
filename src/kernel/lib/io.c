/*
 * @file io.c
 * @brief Low-level I/O port operations
 * @author friedrichOsDev
 */

#include <io.h>

/*
 * Output one byte to the bus
 * @param port The port to write to
 * @param val The value to write
 */
void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read one byte from the bus
 * @param port The port to read from
 * @return The value read from the port
 */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Write two bytes to the bus
 * @param port The port to write to
 * @param val The value to write
 */
void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read two bytes from the bus
 * @param port The port to read from
 * @return The value read from the port
 */
uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Write four bytes to the bus
 * @param port The port to write to
 * @param val The value to write
 */
void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read four bytes from the bus
 * @param port The port to read from
 * @return The value read from the port
 */
uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}