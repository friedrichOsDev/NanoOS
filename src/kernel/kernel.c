#include <kernel.h>
#include <console.h>
#include <print.h>
#include <gdt.h>
#include <idt.h>
#include <irq.h>
#include <handler.h>
#include <heap.h>
#include <log.h>

void kernel_main();

// entry point of the kernel
void __attribute__((section(".text.entry"))) _start() {
    kernel_main();
    while(1);
}

void kernel_main() {
    console_init();
    printf("Welcome to NanoOS kernel!\n");

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
                if (entry->length_low > largest_region_length) {
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

    while(1);
}