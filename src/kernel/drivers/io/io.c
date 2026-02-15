/*
 * @file io.c
 * @brief I/O port driver
 * @author friedrichOsDev
 */

#include <io.h>

 /*
  * Writes a byte to the specified I/O port
  * @param port The I/O port to write to
  * @param value The byte value to write
  */
void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * Reads a byte from the specified I/O port
 * @param port The I/O port to read from
 * @return The byte value read from the port
 */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Writes a word to the specified I/O port
 * @param port The I/O port to write to
 * @param value The word value to write
 */
void outw(uint16_t port, uint16_t value) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * Reads a word from the specified I/O port
 * @param port The I/O port to read from
 * @return The word value read from the port
 */
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Writes a double word to the specified I/O port
 * @param port The I/O port to write to
 * @param value The double word value to write
 */
void outl(uint16_t port, uint32_t value) {
    __asm__ __volatile__ ("outl %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * Reads a double word from the specified I/O port
 * @param port The I/O port to read from
 * @return The double word value read from the port
 */
uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__ ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Writes multiple words from a buffer to the specified I/O port
 * @param port The I/O port to write to
 * @param buffer The source buffer
 * @param count The number of words to write
 */
void outsw(uint16_t port, const void* buffer, uint32_t count) {
    __asm__ __volatile__ ("cld; rep outsw" : "+S"(buffer), "+c"(count) : "d"(port));
}

/*
 * Reads multiple words from the specified I/O port into a buffer
 * @param port The I/O port to read from
 * @param buffer The destination buffer
 * @param count The number of words to read
 */
void insw(uint16_t port, void* buffer, uint32_t count) {
    __asm__ __volatile__ ("cld; rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}
