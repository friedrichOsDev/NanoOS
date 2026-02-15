/*
 * @file kernel.c
 * @brief Main entry point for the NanoOS kernel
 * @author friedrichOsDev
 */

#include <kernel.h>
#include <serial.h>
#include <gdt.h>
#include <idt.h>
#include <irq.h>

/*
 * Kernel entry point
 * @param multiboot_magic The magic number passed by the bootloader
 * @param multiboot_info The address of the multiboot information structure
 */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info) {
    serial_init();

    if (multiboot_info == 0) {
        serial_printf("Error: Multiboot Info structure is NULL! Expected a pointer to the multiboot structure\n");
        while (1);
    }

    if (multiboot_magic != 0x36D76289) {
        serial_printf("Error: Invalid Multiboot Magic! Expected 0x36D76289\n");
        while (1);
    }

    gdt_init();
    idt_init();
    irq_init();
    idt_enable();

    serial_printf("Kernel: Welcome to NanoOS!\n");

    while (1);
}
