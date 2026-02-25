/**
 * @file io.c
 * @author friedrichOsDev
 */

#include <io.h>

/**
 * @brief Sends a byte to an I/O port.
 * 
 * @param port The I/O port address.
 * @param value The byte to send.
 */
void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Reads a byte from an I/O port.
 * 
 * @param port The I/O port address.
 * @return The byte read from the port.
 */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Sends a 16-bit word to an I/O port.
 * 
 * @param port The I/O port address.
 * @param value The word to send.
 */
void outw(uint16_t port, uint16_t value) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Reads a 16-bit word from an I/O port.
 * 
 * @param port The I/O port address.
 * @return The word read from the port.
 */
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Sends a 32-bit double word to an I/O port.
 * 
 * @param port The I/O port address.
 * @param value The double word to send.
 */
void outl(uint16_t port, uint32_t value) {
    __asm__ __volatile__ ("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Reads a 32-bit double word from an I/O port.
 * 
 * @param port The I/O port address.
 * @return The double word read from the port.
 */
uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__ ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Sends a buffer of 16-bit words to an I/O port.
 * 
 * Uses the 'rep outsw' instruction to send multiple words.
 * 
 * @param port The I/O port address.
 * @param buffer Pointer to the source buffer.
 * @param count Number of words to send.
 */
void outsw(uint16_t port, const void* buffer, uint32_t count) {
    __asm__ __volatile__ ("cld; rep outsw" : "+S"(buffer), "+c"(count) : "d"(port));
}

/**
 * @brief Reads a buffer of 16-bit words from an I/O port.
 * 
 * Uses the 'rep insw' instruction to read multiple words.
 * 
 * @param port The I/O port address.
 * @param buffer Pointer to the destination buffer.
 * @param count Number of words to read.
 */
void insw(uint16_t port, void* buffer, uint32_t count) {
    __asm__ __volatile__ ("cld; rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}
