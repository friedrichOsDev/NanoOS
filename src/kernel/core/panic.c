/**
 * @file panic.c
 * @author friedrichOsDev
 */

#include <panic.h>
#include <serial.h>

/**
 * @brief Halts the kernel and output a critical error message.
 * 
 * This function disables interrupts and logs the error to the serial port before entering an infinite loop.
 * 
 * @param error_msg The descriptive error message.
 * @param error_code An optional error code for debugging.
 */
void kernel_panic(const char* error_msg, uint32_t error_code) {
    __asm__ __volatile__("cli");
    serial_printf("KERNEL PANIC: %s (Error code: %x)\n", error_msg, error_code);
    while (1);
}