/*
 * @file panic.c
 * @brief Kernel panic handling
 * @author friedrichOsDev
 */

#include <panic.h>
#include <serial.h>

void kernel_panic(const char* error_msg, uint32_t error_code) {
    serial_printf("KERNEL PANIC: %s (Error code: %x)\n", error_msg, error_code);
    while (1);
}