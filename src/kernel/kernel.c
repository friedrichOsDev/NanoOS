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
        log_info("Allocated int at %x with value %d", (unsigned int)test_ptr1, *test_ptr1);
    } else {
        log_error("Failed to allocate int.");
    }

    char* test_ptr2 = (char*)kmalloc(20 * sizeof(char));
    if (test_ptr2) {
        strcpy(test_ptr2, "Hello, Heap!");
        log_info("Allocated string at %x with value '%s'", (unsigned int)test_ptr2, test_ptr2);
    } else {
        log_error("Failed to allocate string.");
    }

    int* test_ptr3 = (int*)kzalloc(sizeof(int));
    if (test_ptr3) {
        log_info("Allocated zero-initialized int at %x with value %d", (unsigned int)test_ptr3, *test_ptr3);
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
    console_init();
    printf("Welcome to NanoOS kernel!\n\n");

    gdt_init();
    log_success("GDT initialized.");

    idt_init();
    log_success("IDT initialized.");

    irq_init();
    log_success("IRQ initialized.");

    enable_interrupts();
    log_success("Interrupts enabled.");

    // find the largest available memory region for the heap
    uint32_t largest_region_base = 0;
    uint32_t largest_region_length = 0;

    for (int i = 0; i < mmap_info->entry_count; i++) {
        mmap_entry_t* entry = &mmap_info->entries[i];

        if (entry->type == 1) { // type 1 = available RAM
            if (entry->base_addr_high == 0 && entry->length_high == 0) { // high bits must be 0 for 32-bit address space
                // ensure the memory region starts at or above 1MB to avoid low memory areas
                if (entry->base_addr_low >= 0x100000 && entry->length_low > largest_region_length) {
                    largest_region_base = entry->base_addr_low;
                    largest_region_length = entry->length_low;
                }
            }
        }
    }

    if (largest_region_base != 0 && largest_region_length != 0) {
        heap_init((void*)largest_region_base, largest_region_length);
    } else {
        log_warning("No suitable memory region found for heap initialization.");
    }

    log_success("Heap initialized.");

    // test_kernel();

    while(1);
}