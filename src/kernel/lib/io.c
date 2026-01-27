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
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read one byte from the bus
 * @param port The port to read from
 * @return The value read from the port
 */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Write two bytes to the bus
 * @param port The port to write to
 * @param val The value to write
 */
void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read two bytes from the bus
 * @param port The port to read from
 * @return The value read from the port
 */
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Write four bytes to the bus
 * @param port The port to write to
 * @param val The value to write
 */
void outl(uint16_t port, uint32_t val) {
    __asm__ __volatile__ ("outl %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read four bytes from the bus
 * @param port The port to read from
 * @return The value read from the port
 */
uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__ ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Read multiple words from the bus into memory
 * @param port The port to read from
 * @param addr The memory address to store the data
 * @param count The number of words to read
 */
void insw(uint16_t port, void* addr, uint32_t count) {
    __asm__ __volatile__ ("cld; rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

/*
 * Write multiple words from memory to the bus
 * @param port The port to write to
 * @param addr The memory address containing the data
 * @param count The number of words to write
 */
void outsw(uint16_t port, const void* addr, uint32_t count) {
    __asm__ __volatile__ ("cld; rep outsw" : "+S"(addr), "+c"(count) : "d"(port));
}
