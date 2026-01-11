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

void kernel_main();

// entry point of the kernel
void __attribute__((section(".text.entry"))) _start() {
    kernel_main();
    while(1);
}

void test_kernel() {
    printf("\nTest the kernel functionalities:\n\n");

    // Test Logging System
    log_info("This is an info message.");
    log_warning("This is a warning message.");
    log_error("This is an error message.");
    log_debug("This is a debug message.");
    log_success("This is a success message.");
    log_failed("This is a failed message.");

    // Test kmalloc and kfree
    int* test_ptr1 = (int*)kmalloc(sizeof(int));
    if (test_ptr1) {
        *test_ptr1 = 123;
        log_info("Allocated int at 0x%x with value %d", (unsigned int)test_ptr1, *test_ptr1);
    } else {
        log_error("Failed to allocate int.");
    }

    char* test_ptr2 = (char*)kmalloc(20 * sizeof(char));
    if (test_ptr2) {
        strcpy(test_ptr2, "Hello, Heap!");
        log_info("Allocated string at 0x%x with value '%s'", (unsigned int)test_ptr2, test_ptr2);
    } else {
        log_error("Failed to allocate string.");
    }

    int* test_ptr3 = (int*)kzalloc(sizeof(int));
    if (test_ptr3) {
        log_info("Allocated zero-initialized int at 0x%x with value %d", (unsigned int)test_ptr3, *test_ptr3);
    } else {
        log_error("Failed to allocate zero-initialized int.");
    }

    kfree(test_ptr1);
    log_info("Freed test_ptr1.");
    kfree(test_ptr2);
    log_info("Freed test_ptr2.");
    kfree(test_ptr3);
    log_info("Freed test_ptr3.");

    // Test double free detection (should cause a kernel panic)
    // kfree(test_ptr1); 

    // Test invalid pointer free detection (should cause a kernel panic)
    // int invalid_var;
    // kfree(&invalid_var);
}

void kernel_main() {
    serial_init();

    gdt_init();
    serial_puts("gdt_init: done\n");

    idt_init();
    serial_puts("idt_init: done\n");

    irq_init();
    serial_puts("irq_init: done\n");

    enable_interrupts();
    serial_puts("enable_interrupts: done\n");

    heap_init();
    serial_puts("heap_init: done\n");

    console_init();
    printf("Welcome to NanoOS kernel!\n\n");

    timer_init(60);
    serial_puts("timer_init: done\n");

    fb_swap_buffers();

    uint32_t tick = 0;
    uint32_t old_tick = 0;
    uint32_t diff = 0;

    while(1) {
        tick = timer_get_ticks();
        if (tick != old_tick) {
            diff = tick - old_tick;
            old_tick = tick;
            printf("tick %d, diff: %d\n", tick, diff);
            fb_swap_buffers();
        }
    };
}