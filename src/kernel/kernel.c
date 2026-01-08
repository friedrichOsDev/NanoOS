#include <kernel.h>
#include <console.h>
#include <print.h>
#include <gdt.h>
#include <idt.h>
#include <irq.h>
#include <handler.h>
#include <heap.h>

void kernel_main();

// entry point of the kernel
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
        printf("WARNING: No suitable memory region found for heap initialization.\n");
    }

    printf("Heap initialized: %kSuccess%k\n", 0x00FF00, 0xFFFFFF);

    while(1);
}