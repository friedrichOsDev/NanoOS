/**
 * @file init.c
 * @brief Kernel initialization code
 * @author friedrichOsDev
 */

#include <core/init.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/cpu/gdt.h>

void kernel_init(const uint64_t magic, const uint64_t info_ptr) {
    serial_init(COM1);
    serial_printf(COM1, "INIT: start\n");
    serial_printf(COM1, "MULTIBOOT2: magic=%x\n", magic);
    serial_printf(COM1, "MULTIBOOT2: info_ptr=%x\n", info_ptr);

    gdt_init();

    serial_printf(COM1, "INIT: done\n");

    while (1) __asm__("hlt");
}
