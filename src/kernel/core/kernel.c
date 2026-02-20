/*
 * @file kernel.c
 * @brief Main entry point for the NanoOS kernel
 * @author friedrichOsDev
 */

#include <kernel.h>
#include <serial.h>
#include <gdt.h>
#include <idt.h>
#include <irq.h>
#include <panic.h>
#include <pmm.h>
#include <vmm.h>
#include <heap.h>

mmap_t kernel_mmap;
fb_info_t kernel_fb_info;
multiboot_info_t* kernel_multiboot_info;

/*
 * Parse the multiboot information structure provided by the bootloader
 * @param multiboot_magic The magic number passed by the bootloader
 * @param multiboot_info The address of the multiboot information structure
 */
void multiboot_parse(uint32_t multiboot_magic, uint32_t multiboot_info) {
    if (multiboot_info == 0) kernel_panic("Multiboot info structure is missing!", 0);
    if (multiboot_magic != 0x36D76289) kernel_panic("Invalid multiboot magic number! Expected 0x36D76289", 0);

    kernel_multiboot_info = (multiboot_info_t*)multiboot_info;
    serial_printf("Multiboot: Info at %x with size %d\n", multiboot_info, kernel_multiboot_info->total_size);

    multiboot_tag_t* tag = (multiboot_tag_t*)(multiboot_info + sizeof(multiboot_info_t));

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        serial_printf("Multiboot: Tag type: %d, size: %d\n", tag->type, tag->size);
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                multiboot_tag_cmdline_t* cmdline_tag = (multiboot_tag_cmdline_t*)tag;
                if (cmdline_tag->size > 8 && cmdline_tag->string[0] != '\0') {
                    serial_printf("Multiboot: Command line: '%s'\n", cmdline_tag->string);
                } else {
                    serial_printf("Multiboot: Command line: (empty)\n");
                }
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER:
                multiboot_tag_boot_loader_t* boot_loader_tag = (multiboot_tag_boot_loader_t*)tag;
                serial_printf("Multiboot: Boot loader name: %s\n", boot_loader_tag->string);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                multiboot_tag_mmap_t* mmap_tag = (multiboot_tag_mmap_t*)tag;
                kernel_mmap.entry_count = 0;
                for (multiboot_tag_mmap_entry_t* entry = mmap_tag->entries; (uint8_t*)entry < (uint8_t*)tag + tag->size; entry = (multiboot_tag_mmap_entry_t*)((uintptr_t)entry + mmap_tag->entry_size)) {
                    if (kernel_mmap.entry_count < MMAP_MAX_ENTRIES) {
                        kernel_mmap.entries[kernel_mmap.entry_count].base_addr = entry->base_addr;
                        kernel_mmap.entries[kernel_mmap.entry_count].length = entry->length;
                        kernel_mmap.entries[kernel_mmap.entry_count].type = (mmap_type_t)entry->type;
                        kernel_mmap.entry_count++;
                    }
                    serial_printf("Multiboot: Memory region: base=%x:%x, len=%x:%x, type=%d\n", (uint32_t)(entry->base_addr >> 32), (uint32_t)entry->base_addr, (uint32_t)(entry->length >> 32), (uint32_t)entry->length, entry->type);
                }
                break;
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
                multiboot_tag_framebuffer_t* fb_tag = (multiboot_tag_framebuffer_t*)tag;
                kernel_fb_info.fb_addr = (void*)(uintptr_t)fb_tag->framebuffer_addr;
                kernel_fb_info.fb_width = fb_tag->framebuffer_width;
                kernel_fb_info.fb_height = fb_tag->framebuffer_height;
                kernel_fb_info.fb_pitch = fb_tag->framebuffer_pitch;
                kernel_fb_info.fb_bpp = fb_tag->framebuffer_bpp;
                
                serial_printf("Multiboot: Framebuffer: %dx%dx%d at %x, type: %d\n", fb_tag->framebuffer_width, fb_tag->framebuffer_height, fb_tag->framebuffer_bpp, (uint32_t)fb_tag->framebuffer_addr, fb_tag->framebuffer_type);
                
                if (fb_tag->framebuffer_type == 2) kernel_panic("Unsupported framebuffer type: EGA text mode is not supported", 0);
                break;
            default: break;
        }
        tag = (multiboot_tag_t*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
}

/*
 * Test the kernel functionality
 * @param void
 */
void kernel_tests(void) {
    // idt
    // serial_printf("Kernel: Tests: IDT: Triggering division by zero exception to test IDT setup...\n");
    // int a = 5;
    // int b = 0;
    // int c = a / b;
    // serial_printf("Kernel: Tests: IDT: Division by zero test: 5 / 0 = %d (should not be reached)\n", c);

    // memory management test
    // - pmm
    serial_printf("-----\n");

    serial_printf("Kernel: Tests: PMM: Total memory: %d KiB\n", (uint32_t)pmm_get_total_memory() / 1024);
    serial_printf("Kernel: Tests: PMM: Used memory: %d KiB\n", (uint32_t)pmm_get_used_memory() / 1024);
    serial_printf("Kernel: Tests: PMM: Free memory: %d KiB\n", (uint32_t)pmm_get_free_memory() / 1024);

    serial_printf("-----\n");

    phys_addr_t test_alloc = pmm_zalloc_pages(1024);
    serial_printf("Kernel: Tests: PMM: Allocated 1024 pages at %x\n", test_alloc);
    serial_printf("Kernel: Tests: PMM: Used memory: %d KiB\n", (uint32_t)pmm_get_used_memory() / 1024);
    pmm_zfree_pages(test_alloc, 1024);
    serial_printf("Kernel: Tests: PMM: Freed 1024 pages at %x\n", test_alloc);
    serial_printf("Kernel: Tests: PMM: Used memory: %d KiB\n", (uint32_t)pmm_get_used_memory() / 1024);

    serial_printf("-----\n");

    // - vmm
    virt_addr_t test_virtual_addr = 0x400000; // 4 MiB
    phys_addr_t test_physical_addr = pmm_alloc_page();
    serial_printf("Kernel: Tests: VMM: Mapping virtual %x to physical %x\n", test_virtual_addr, test_physical_addr);
    vmm_map_page(vmm_get_page_directory(), test_virtual_addr, test_physical_addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);

    serial_printf("Kernel: Tests: VMM: Verifying mapping...\n");
    phys_addr_t translated = vmm_virtual_to_physical(vmm_get_page_directory(), test_virtual_addr);
    if (translated == test_physical_addr) {
        serial_printf("Kernel: Tests: VMM: SUCCESS: %x -> %x\n", test_virtual_addr, translated);
    } else {
        serial_printf("Kernel: Tests: VMM: FAILURE: %x -> %x (expected %x)\n", test_virtual_addr, translated, test_physical_addr);
    }

    // test mapping second phys to same virt
    virt_addr_t test_virtual_addr2 = 0x400000 - VMM_PAGE_SIZE; // 4 MiB - 4 KiB
    phys_addr_t test_physical_addr2 = pmm_alloc_pages(2);
    serial_printf("Kernel: Tests: VMM: Attempting to map 2 physical pages at %x to overlap with the virtual address %x (should fail)\n", test_physical_addr2, test_virtual_addr);
    vmm_map_pages(vmm_get_page_directory(), test_virtual_addr2, test_physical_addr2, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, 2);
    pmm_free_pages(test_physical_addr2, 2);

    serial_printf("Kernel: Tests: VMM: Unmapping %x\n", test_virtual_addr);
    vmm_unmap_page(vmm_get_page_directory(), test_virtual_addr);
    pmm_free_page(test_physical_addr);

    serial_printf("-----\n");

    // - heap
    serial_printf("Kernel: Tests: Heap: Allocating 128 bytes...\n");
    virt_addr_t ptr1 = kmalloc(128);
    serial_printf("Kernel: Tests: Heap: Allocating 256 bytes...\n");
    virt_addr_t ptr2 = kmalloc(256);
    serial_printf("Kernel: Tests: Heap: Allocating 512 bytes...\n");
    virt_addr_t ptr3 = kmalloc(512);
    serial_printf("Kernel: Tests: Heap: Allocated 3 blocks: %x, %x, %x\n", ptr1, ptr2, ptr3);
    heap_dump();

    serial_printf("Kernel: Tests: Heap: Freeing block 2 (256 bytes) to test Best-Fit...\n");
    kfree(ptr2);
    heap_dump();

    virt_addr_t ptr_fit = kmalloc(128);
    serial_printf("Kernel: Tests: Heap: New 128 byte block allocated at: %x\n", ptr_fit);
    if (ptr_fit == ptr2) {
        serial_printf("Kernel: Tests: Heap: SUCCESS: Best-Fit picked the hole at %x\n", ptr2);
    }
    heap_dump();

    serial_printf("Kernel: Tests: Heap: Testing Heap extension by allocating a large block of 2 MiB...\n");
    virt_addr_t ptr_large = kmalloc(2 * 1024 * 1024);
    serial_printf("Kernel: Tests: Heap: Large block allocated at: %x\n", ptr_large);
    heap_dump();

    serial_printf("Kernel: Tests: Heap: Freeing all blocks to test Coalescing...\n");
    kfree(ptr1);
    kfree(ptr_fit);
    kfree(ptr3);
    kfree(ptr_large);
    heap_dump();

    serial_printf("-----\n");
}

/*
 * Kernel entry point
 * @param multiboot_magic The magic number passed by the bootloader
 * @param multiboot_info The address of the multiboot information structure
 */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info) {
    serial_init();
    gdt_init();
    idt_init();
    irq_init();
    idt_enable();

    multiboot_parse(multiboot_magic, multiboot_info);

    pmm_init();
    vmm_init();
    heap_init();

    kernel_tests();

    serial_printf("Kernel: Welcome to NanoOS!\n");

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
