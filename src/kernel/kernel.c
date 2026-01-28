/*
 * @file kernel.c
 * @brief Kernel main source file
 * @author friedrichOsDev
 */

#include <kernel.h>
#include <console.h>
#include <print.h>
#include <gdt.h>
#include <idt.h>
#include <irq.h>
#include <handler.h>
#include <heap.h>
#include <log.h>
#include <string.h>
#include <serial.h>
#include <fb.h>
#include <timer.h>
#include <cpu.h>
#include <paging.h>
#include <keyboard.h>
#include <shell.h>
#include <rtc.h>
#include <acpi.h>
#include <ata.h>
#include <pci.h>

/*
 * TODO: implement dynamic kernel loading in bootloader stage 3
 */

/*
 * Kernel entry point
 * @param void
 * @note This function is linked to the .text.entry section (aka start of the kernel binary)
 */
void __attribute__((section(".text.entry"))) _start(void) {
    kernel_main();
    while(1);
}

/*
 * Kernel main function
 * @param void
 */
void kernel_main(void) {
    serial_init();
    serial_puts("serial_init: done\n");

    gdt_init();
    serial_puts("gdt_init: done\n");

    idt_init();
    serial_puts("idt_init: done\n");

    irq_init();
    serial_puts("irq_init: done\n");

    cpu_enable_interrupts();
    serial_puts("cpu_enable_interrupts: done\n");

    heap_init();
    serial_puts("heap_init: done\n");

    paging_init();
    serial_puts("paging_init: done\n");

    console_init();
    serial_puts("console_init: done\n");

    acpi_init();
    serial_puts("acpi_init: done\n");

    timer_init(60);
    serial_puts("timer_init: done\n");

    keyboard_init("de");
    serial_puts("keyboard_init: done\n");

    rtc_init();
    serial_puts("rtc_init: done\n");

    pci_init();
    serial_puts("pci_init: done\n");

    shell_init();
    serial_puts("shell_init: done\n");

    uint32_t tick = 0;
    uint32_t old_tick = 0;

    while(1) {
        tick = timer_get_ticks();
        if (tick != old_tick) {
            old_tick = tick;
            // this block runs every tick (about 60 times per second)
            rtc_update_time();
            shell_handle_input(keyboard_getchar());
            fb_swap_buffers();
        }
    }
}