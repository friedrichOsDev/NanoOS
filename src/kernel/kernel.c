#include "include/kernel.h"
#include "include/console.h"

void kernel_main();

// Force this function to be in a special section called ".text.entry"
void __attribute__((section(".text.entry"))) _start() {
    kernel_main();
    while(1);
}

void kernel_main() {
    console_init();

    console_set_color(0x00FF00, 0x000000); // Green text on black background
    console_puts("Welcome to the Kernel Console!\n");
    console_set_color(0xFF0000, 0x000000); // Red text on black background
    console_puts("This is a simple text output using framebuffer.\n");

    while(1);
}