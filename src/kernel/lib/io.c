/**
 * @file io.c
 * @brief I/O Port Implementation
 * @author friedrichOsDev
 */

#include <lib/io.h>

void outb(uint16_t port, uint8_t val) {
    ASM_CODE("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    ASM_CODE("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

void outw(uint16_t port, uint16_t val) {
    ASM_CODE("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    ASM_CODE("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

void outl(uint16_t port, uint32_t val) {
    ASM_CODE("outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    ASM_CODE("inl %w1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}
