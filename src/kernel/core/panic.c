/**
 * @file panic.c
 * @brief Kernel panic
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/panic.h>

/**
 * This is a Kernel Panic
 * @param message The error message
 * @param error_code The error code (if not available it should be 0)
 */
void panic(const char *message, uint64_t error_code) {
    idt_disable();
    serial_printf(COM1, "KERNEL PANIC: %s (Error code %llx)\n", message,
                  error_code);
    while (1)
        __asm__ __volatile__("hlt");
}