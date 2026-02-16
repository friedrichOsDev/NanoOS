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

mmap_t kernel_mmap;
fb_info_t kernel_fb_info;

/*
 * Parse the multiboot information structure provided by the bootloader
 * @param multiboot_magic The magic number passed by the bootloader
 * @param multiboot_info The address of the multiboot information structure
 */
void multiboot_parse(uint32_t multiboot_magic, uint32_t multiboot_info) {
    if (multiboot_info == 0) {
        serial_printf("Error: Multiboot Info structure is NULL! Expected a pointer to the multiboot structure\n");
        while (1);
    }

    if (multiboot_magic != 0x36D76289) {
        serial_printf("Error: Invalid Multiboot Magic! Expected 0x36D76289\n");
        while (1);
    }

    multiboot_info_t* mbi = (multiboot_info_t*)multiboot_info;
    serial_printf("Multiboot: Info at %x with size %d\n", multiboot_info, mbi->total_size);

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
                    serial_printf("Multiboot: Memory region: base=%x:%x, len=%x:%x, type=%d\n",
                        (uint32_t)(entry->base_addr >> 32), (uint32_t)entry->base_addr,
                        (uint32_t)(entry->length >> 32), (uint32_t)entry->length,
                        entry->type);
                }
                break;
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
                multiboot_tag_framebuffer_t* fb_tag = (multiboot_tag_framebuffer_t*)tag;
                kernel_fb_info.fb_addr = (void*)(uintptr_t)fb_tag->framebuffer_addr;
                kernel_fb_info.fb_width = fb_tag->framebuffer_width;
                kernel_fb_info.fb_height = fb_tag->framebuffer_height;
                kernel_fb_info.fb_pitch = fb_tag->framebuffer_pitch;
                kernel_fb_info.fb_bpp = fb_tag->framebuffer_bpp;
                
                serial_printf("Multiboot: Framebuffer: %dx%dx%d at %x, type: %d\n", 
                    fb_tag->framebuffer_width, fb_tag->framebuffer_height, fb_tag->framebuffer_bpp, 
                    (uint32_t)fb_tag->framebuffer_addr, fb_tag->framebuffer_type);
                
                if (fb_tag->framebuffer_type == 2) {
                    serial_printf("Multiboot: Warning: Framebuffer is in TEXT MODE (0xB8000). Update Multiboot Header!\n");
                }
                break;
            default: break;
        }
        tag = (multiboot_tag_t*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
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

    serial_printf("Kernel: Welcome to NanoOS!\n");

    while (1);
}
