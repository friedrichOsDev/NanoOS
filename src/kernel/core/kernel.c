/*
 * @file kernel.c
 * @brief Main entry point for the NanoOS kernel
 * @author friedrichOsDev
 */

#include <kernel.h>
#include <serial.h>

/*
 * Kernel entry point
 * @param multiboot_magic The magic number passed by the bootloader
 * @param multiboot_info The address of the multiboot information structure
 */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info) {
    // Initialize the serial port for debugging
    serial_init();

    // Print a welcome message
    serial_printf("Welcome to NanoOS!\n");
    serial_printf("Kernel initialized successfully.\n");
    
    // Print debug information about the multiboot magic
    serial_printf("Multiboot Magic: %x\n", multiboot_magic);
    serial_printf("Multiboot Info:  %x\n", multiboot_info);

    if (multiboot_info == 0) {
        serial_printf("Warning: Multiboot Info structure is NULL!\n");
    }

    if (multiboot_magic != 0x36D76289) {
        serial_printf("Error: Invalid Multiboot Magic! Expected 0x36D76289\n");
    }

    while (1);
}
