#include "include/kernel.h"
#include "include/console.h"
#include "include/print.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/irq.h"
#include "include/handler.h"

void kernel_main();

// Force this function to be in a special section called ".text.entry"
void __attribute__((section(".text.entry"))) _start() {
    kernel_main();
    while(1);
}

void kernel_main() {
    console_init();
    printf("Welcome to NanoOS kernel!\n", 0x00FF00, 0xFFFFFF);

    gdt_init();
    printf("GDT initialized: %kSuccess%k\n", 0x00FF00, 0xFFFFFF);

    idt_init();
    printf("IDT initialized: %kSuccess%k\n", 0x00FF00, 0xFFFFFF);

    irq_init();
    printf("IRQ initialized: %kSuccess%k\n", 0x00FF00, 0xFFFFFF);

    enable_interrupts();
    printf("Interrupts enabled: %kSuccess%k\n", 0x00FF00, 0xFFFFFF);

    while(1);
}