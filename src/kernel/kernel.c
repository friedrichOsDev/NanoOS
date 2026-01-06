#include "include/kernel.h"
#include "include/console.h"
#include "include/print.h"

void kernel_main();


// Force this function to be in a special section called ".text.entry"
void __attribute__((section(".text.entry"))) _start() {
    kernel_main();
    while(1);
}

void kernel_main() {
    console_init();

    printf("Welcome %k%Kto%k%K the Kernel!\n\n", 0x00FF00, 0x000000, 0xFFFFFF, 0x000000);
    printf("Colors: %x, %x, %x, %x\n\n", 0x00FF00, 0x000000, 0xFFFFFF, 0x000000);
    printf("Characters: %c, %c, %c, %k%c%k\n\n", 'A', 'B', 'C', 0xee22ff, 'D', 0xFFFFFF);
    printf("Decimal numbers: %d, %k%d%k, %d, %d\n\n", 42, 0xFF0000, -42, 0xFFFFFF, 0, 123456);
    printf("Hexadecimal numbers: %x, %x, %x, %x\n\n", 0x1234ABCD, 0xDEADBEEF, 0x0, 0xFF);
    printf("Mixed: %k%s%k %d %x %c\n\n", 0x0000FF, "Hello", 0xFFFFFF, 123, 0xABC, 'X');

    while(1);
}