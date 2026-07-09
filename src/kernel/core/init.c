/**
 * @file init.c
 * @brief Kernel initialization code
 * @author friedrichOsDev
 */

#include <core/init.h>
#include <arch/x86_64/drivers/serial.h>

void kernel_init(const uint64_t magic, const uint64_t info_ptr) {
    serial_init(COM1);
    serial_printf(COM1, "Hello From 64-Bit Kernel!\n");
    serial_printf(COM1, "Multiboot2: Info Ptr: %p\n            Magic: %x\n", info_ptr, magic);

    while (1) __asm__("hlt");
}