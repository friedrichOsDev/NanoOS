/**
 * @file init.c
 * @brief Kernel initialization code
 * @author friedrichOsDev
 */

#include <core/init.h>
#include <core/panic.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/cpu/irq.h>
#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/mm/pmm.h>
#include <arch/x86_64/mm/vmm.h>

mmap_t kernel_mmap;
fb_info_t kernel_fb_info;
multiboot_info_t* kernel_multiboot_info;
char kernel_cmdline[256];
char kernel_bootloader_name[64];

static void multiboot_parse(const uint64_t magic, const uint64_t info_ptr) {
    if (magic != MULTIBOOT2_MAGIC) {
        panic("bad multiboot2 magic", magic);
    } else if (!info_ptr) {
        panic("bad multiboot2 info_ptr", info_ptr);
    }

    kernel_multiboot_info = (multiboot_info_t*)info_ptr;
    multiboot_tag_t* tag = (multiboot_tag_t*)(info_ptr + sizeof(multiboot_info_t));

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        serial_printf(COM1, "MULTIBOOT2: type=%x, size=%x\n", tag->type, tag->size);
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_CMDLINE: {
                multiboot_tag_cmdline_t* cmdline = (multiboot_tag_cmdline_t *) tag;
                if (cmdline->size > 8 && cmdline->string[0] != '\0') {
                    uint32_t len = cmdline->size - 8;
                    if (len > sizeof(kernel_cmdline) - 1) len = sizeof(kernel_cmdline) - 1;
                    for (uint32_t i = 0; i < len; i++) kernel_cmdline[i] = cmdline->string[i];
                    kernel_cmdline[len] = '\0';
                    serial_printf(COM1, "MULTIBOOT2: cmdline=%s\n", kernel_cmdline);
                } else {
                    serial_printf(COM1, "MULTIBOOT2: cmdline=NULL\n");
                }
                break;
            }
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER: {
                multiboot_tag_boot_loader_t* boot_loader = (multiboot_tag_boot_loader_t*)tag;
                if (boot_loader->size > 8 && boot_loader->string[0] != '\0') {
                    uint32_t len = boot_loader->size - 8;
                    if (len > sizeof(kernel_bootloader_name) - 1) len = sizeof(kernel_bootloader_name) - 1;
                    for (uint32_t i = 0; i < len; i++) kernel_bootloader_name[i] = boot_loader->string[i];
                    kernel_bootloader_name[len] = '\0';
                    serial_printf(COM1, "MULTIBOOT2: bootloader=%s\n", kernel_bootloader_name);
                } else {
                    serial_printf(COM1, "MULTIBOOT2: bootloader=NULL\n");
                }
                break;
            }
            case MULTIBOOT_TAG_TYPE_MODULE: {
                multiboot_tag_module_t* module = (multiboot_tag_module_t*)tag;
                (void)module;
                serial_printf(COM1, "MULTIBOOT2: module isn't implemented yet\n");
                break;
            }
            case MULTIBOOT_TAG_TYPE_MMAP: {
                multiboot_tag_mmap_t* mmap = (multiboot_tag_mmap_t*)tag;
                kernel_mmap.entry_count = 0;
                for (multiboot_tag_mmap_entry_t* entry = mmap->entries; (uint8_t*)entry < (uint8_t*)tag + tag->size; entry = (multiboot_tag_mmap_entry_t*)((uintptr_t)entry + mmap->entry_size)) {
                    if (kernel_mmap.entry_count < MMAP_MAX_ENTRIES) {
                        kernel_mmap.entries[kernel_mmap.entry_count].base_addr = entry->base_addr;
                        kernel_mmap.entries[kernel_mmap.entry_count].length = entry->length;
                        kernel_mmap.entries[kernel_mmap.entry_count].type = (mmap_type_t)entry->type;
                        kernel_mmap.entry_count++;
                    }
                    serial_printf(COM1, "MULTIBOOT2: mregion: base=%llx, len=%llx, type=%d\n", entry->base_addr, entry->length, entry->type);
                }
                break;
            }
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
                multiboot_tag_framebuffer_t* framebuffer = (multiboot_tag_framebuffer_t*)tag;
                kernel_fb_info.fb_addr = framebuffer->framebuffer_addr;
                kernel_fb_info.fb_width = framebuffer->framebuffer_width;
                kernel_fb_info.fb_height = framebuffer->framebuffer_height;
                kernel_fb_info.fb_pitch = framebuffer->framebuffer_pitch;
                kernel_fb_info.fb_bpp = framebuffer->framebuffer_bpp;
                serial_printf(COM1, "MULTIBOOT2: framebuffer: %dx%dx%d at %llx, type=%d\n", framebuffer->framebuffer_width, framebuffer->framebuffer_height, framebuffer->framebuffer_bpp, framebuffer->framebuffer_addr, framebuffer->framebuffer_type);
                if (framebuffer->framebuffer_type == 2) panic("Unsupported framebuffer type: EGA text mode is not supported", framebuffer->framebuffer_type);
                break;
            }
            case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
                multiboot_tag_old_acpi_t* old = (multiboot_tag_old_acpi_t*)tag;
                (void)old;
                serial_printf(COM1, "MULTIBOOT2: old acpi isn't implemented yet\n");
                break;
            }
            case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
                multiboot_tag_new_acpi_t* new = (multiboot_tag_new_acpi_t*)tag;
                (void)new;
                serial_printf(COM1, "MULTIBOOT2: new acpi isn't implemented yet\n");
                break;
            }
            default: {
                serial_printf(COM1, "MULTIBOOT2: unknown type %d\n", tag->type);
                break;
            }
        }
        tag = (multiboot_tag_t*)((uint8_t*)tag + ((tag->size + 7) & ~0x7));
    }
}

void kernel_init(const uint64_t magic, const uint64_t info_ptr) {
    serial_init(COM1);
    serial_printf(COM1, "INIT: start\n");

    gdt_init();
    idt_init();
    irq_init();

    idt_enable();

    serial_printf(COM1, "MULTIBOOT2: magic=%x\n", magic);
    serial_printf(COM1, "MULTIBOOT2: info_ptr=%x\n", info_ptr);

    multiboot_parse(magic, info_ptr);

    pmm_init();
    vmm_init();

    serial_printf(COM1, "INIT: done\n");

    while (1) __asm__("hlt");
}
