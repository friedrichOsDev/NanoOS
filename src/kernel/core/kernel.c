/*
 * @file kernel.c
 * @brief Main entry point for the NanoOS kernel
 * @author friedrichOsDev
 */

#include <kernel.h>

/*
 * Kernel entry point
 * @param multiboot_magic The magic number passed by the bootloader
 * @param multiboot_info The address of the multiboot information structure
 */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info) {
    // Prevent unused parameter warnings
    (void)multiboot_magic;
    (void)multiboot_info;

    while (1);
}
