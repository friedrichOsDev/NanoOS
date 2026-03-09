/**
 * @file kernel.c
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
#include <fb.h>
#include <console.h>
#include <convert.h>
#include <timer.h>
#include <rtc.h>
#include <print.h>
#include <layouts.h>
#include <i8042.h>
#include <keyboard.h>
#include <shell.h>

init_state_t init_state = INIT_START;
mmap_t kernel_mmap;
fb_info_t kernel_fb_info;
multiboot_info_t* kernel_multiboot_info;
char kernel_cmdline[256];
char kernel_bootloader_name[64];

/**
 * @brief Parses the Multiboot2 information structure.
 *
 * Iterates through the Multiboot2 tags to extract command line, bootloader name,
 * memory map, and framebuffer information.
 * @param multiboot_magic The magic value passed by the bootloader.
 * @param multiboot_info The physical address of the Multiboot2 information structure.
 */
void multiboot_parse(uint32_t multiboot_magic, uint32_t multiboot_info) {
    if (multiboot_info == 0) kernel_panic("Multiboot info structure is missing!", 0);
    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) kernel_panic("Invalid multiboot magic number!", 0);

    kernel_multiboot_info = (multiboot_info_t*)multiboot_info;
    serial_printf("Multiboot: Info at %x with size %d\n", multiboot_info, kernel_multiboot_info->total_size);

    multiboot_tag_t* tag = (multiboot_tag_t*)(multiboot_info + sizeof(multiboot_info_t));

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        serial_printf("Multiboot: Tag type: %d, size: %d\n", tag->type, tag->size);
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                multiboot_tag_cmdline_t* cmdline_tag = (multiboot_tag_cmdline_t*)tag;
                if (cmdline_tag->size > 8 && cmdline_tag->string[0] != '\0') {
                    size_t len = cmdline_tag->size - 8;
                    if (len > sizeof(kernel_cmdline) - 1) len = sizeof(kernel_cmdline) - 1;
                    for (size_t i = 0; i < len; i++) kernel_cmdline[i] = cmdline_tag->string[i];
                    kernel_cmdline[len] = '\0';

                    serial_printf("Multiboot: Command line: '%s'\n", cmdline_tag->string);
                } else {
                    serial_printf("Multiboot: Command line: (empty)\n");
                }
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER:
                multiboot_tag_boot_loader_t* boot_loader_tag = (multiboot_tag_boot_loader_t*)tag;
                size_t bl_len = boot_loader_tag->size - 8;
                if (bl_len > sizeof(kernel_bootloader_name) - 1) bl_len = sizeof(kernel_bootloader_name) - 1;
                for (size_t i = 0; i < bl_len; i++) kernel_bootloader_name[i] = boot_loader_tag->string[i];
                kernel_bootloader_name[bl_len] = '\0';

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
    init_state = INIT_MULTIBOOT;
}

/**
 * @brief Executes kernel-level tests and demonstrations.
 * 
 * This function is used to verify kernel components during development.
 */
void kernel_tests(void) {
    heap_dump();
    dump_layout(&scancode_to_vk_map, &vk_to_unicode_de);
}

/**
 * @brief The main entry point of the kernel.
 *
 * This function initializes all core kernel subsystems, parses multiboot information,
 * sets up memory management, and enters the main kernel loop.
 * @param multiboot_magic The magic value passed by the bootloader.
 * @param multiboot_info The physical address of the Multiboot2 information structure.
 */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info) {
    serial_init();
    gdt_init();
    idt_init();
    irq_init();

    idt_enable();
    init_state = INIT_INTERRUPTS;

    multiboot_parse(multiboot_magic, multiboot_info);

    pmm_init();
    vmm_init();
    heap_init();

    timer_init();
    rtc_init();

    // We have an emulated PS/2 controller so the initialization does not work
    // i8042_init();

    keyboard_init(&vk_to_unicode_de);
    
    console_init(
        8,
        8,
        fb_get_width() - 16,
        fb_get_height() - 16,
        (font_color_t){
            .bg_color = (color_t){
                .a = 255,
                .r = 0,
                .g = 10,
                .b = 0
            },
            .fg_color = (color_t){
                .a = 255,
                .r = 100,
                .g = 255,
                .b = 100
            }
        }
    );

    
    init_state = INIT_DONE;

    kernel_tests();

    // printf(U"\n Welcome to NanoOS!\n");
    // const uint32_t* welcome_window = 
    // U"\n"
    // U"  ┌──────────────────────────────┐\n"
    // U"  │         Welcome to           │\n"
    // U"  │   _  _                       │\n"
    // U"  │  | \\| |__ _ _ _  ___         │\n"
    // U"  │  | .` / _` | ' \\/ _ \\        │\n"
    // U"  │  |_|\\_\\__,_|_||_\\___/  os    │\n"
    // U"  │                              │\n"
    // U"  │  Unicode: Σ σ β äöü € ∞      │\n"
    // U"  └──────────────────────────────┘\n"
    // U"   ████████████████████░░░░░░░░░░ 66%\n"
    // U"\n";
    // console_puts(welcome_window);

    serial_printf("Kernel: Welcome to NanoOS!\n");

    shell_init();

    while (1) {
        uint32_t unicode = keyboard_get_unicode();
        shell_handle_input(unicode);

        console_update();
        fb_update();
        
        __asm__ __volatile__("hlt");
    }
}
