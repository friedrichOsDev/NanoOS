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

void kernel_main();

// entry point of the kernel
void __attribute__((section(".text.entry"))) _start() {
    kernel_main();
    while(1);
}

void kernel_main() {
    serial_init();

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

    timer_init(60);
    serial_puts("timer_init: done\n");

    keyboard_init("de");
    serial_puts("keyboard_init: done\n");

    shell_init();
    serial_puts("shell_init: done\n");

    uint32_t tick = 0;
    uint32_t old_tick = 0;

    while(1) {
        tick = timer_get_ticks();
        if (tick != old_tick) {
            old_tick = tick;
            // this block runs every tick (about 60 times per second)
            shell_handle_input(keyboard_getchar());
            fb_swap_buffers();
        }
    }
}