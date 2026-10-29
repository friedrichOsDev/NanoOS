#include "../acpi/acpi.h"
#include "../mm/mem_def.h"
#include "_multiboot2.h"
#include <arch/arch.h>
#include <boot/multiboot2.h>
#include <core/init.h>
#include <kernel/panic.h>
#include <lib/util/mem.h>
#include <lib/util/str.h>
#include <pch.h>

// externs
mmap_t kernel_mmap;
fb_info_t kernel_fb_info;
char kernel_cmdline[256];
char kernel_bootloader_name[64];
boot_modules_t kernel_modules;
phys_addr_t rsdp_phys_addr = 0;

static rsdp_t rsdp_stable_copy;
static multiboot_info_t *kernel_multiboot_info;

/**
 * @brief Liest und verarbeitet die Multiboot2-Tags des Bootloaders.
 * @details Extrahiert Kernel-Befehlszeile, Bootloader-Namen, geladene Module,
 *          Speichermap, Framebuffer-Konfiguration und ACPI-RSDP-Zeiger.
 * @param magic Die von GRUB/Multiboot übergebene Magic Number.
 * @param info_ptr Physikalische Adresse der Multiboot-Tags.
 */
void multiboot_parse(const uint64_t magic, const uint64_t info_ptr) {
    if (magic != MULTIBOOT2_MAGIC) {
        panic();
    } else if (!info_ptr) {
        panic();
    }

    kernel_multiboot_info = (multiboot_info_t *)info_ptr;

    uint32_t total_size = kernel_multiboot_info->total_size;
    uint8_t *end_addr = (uint8_t *)info_ptr + total_size;

    multiboot_tag_t *tag = (multiboot_tag_t *)(info_ptr + sizeof(multiboot_info_t));

    while ((uint8_t *)tag < end_addr && tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->size == 0) {
            serial_printf("MULTIBOOT2: Corrupt tag with size 0\n");
            break;
        }

        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE: {
            multiboot_tag_cmdline_t *cmdline = (multiboot_tag_cmdline_t *)tag;
            strncpy(kernel_cmdline, cmdline->string, sizeof(kernel_cmdline) - 1);
            kernel_cmdline[sizeof(kernel_cmdline) - 1] = '\0';
            serial_printf("MULTIBOOT2: cmdline=%s\n", kernel_cmdline);
            break;
        }
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER: {
            multiboot_tag_boot_loader_t *boot_loader = (multiboot_tag_boot_loader_t *)tag;
            strncpy(kernel_bootloader_name, boot_loader->string, sizeof(kernel_bootloader_name) - 1);
            kernel_bootloader_name[sizeof(kernel_bootloader_name) - 1] = '\0';
            serial_printf("MULTIBOOT2: bootloader=%s\n", kernel_bootloader_name);
            break;
        }
        case MULTIBOOT_TAG_TYPE_MODULE: {
            multiboot_tag_module_t *module = (multiboot_tag_module_t *)tag;
            if (kernel_modules.count < MAX_MODULES) {
                boot_module_t *entry = &kernel_modules.entries[kernel_modules.count++];
                entry->mod_start = module->mod_start;
                entry->mod_end = module->mod_end;
                strncpy(entry->cmdline, module->cmdline, sizeof(entry->cmdline) - 1);
                entry->cmdline[sizeof(entry->cmdline) - 1] = '\0';
                serial_printf("MULTIBOOT2: module at [%x - %x] cmd=%s\n", entry->mod_start, entry->mod_end, entry->cmdline);
            } else {
                serial_printf("MULTIBOOT2: warning, max modules reached\n");
            }
            break;
        }
        case MULTIBOOT_TAG_TYPE_MMAP: {
            multiboot_tag_mmap_t *mmap = (multiboot_tag_mmap_t *)tag;
            kernel_mmap.entry_count = 0;
            for (multiboot_tag_mmap_entry_t *entry = mmap->entries;
                 (uint8_t *)entry < (uint8_t *)tag + tag->size;
                 entry = (multiboot_tag_mmap_entry_t *)((uintptr_t)entry + mmap->entry_size)) {
                if (kernel_mmap.entry_count < MMAP_MAX_ENTRIES) {
                    kernel_mmap.entries[kernel_mmap.entry_count].base_addr = entry->base_addr;
                    kernel_mmap.entries[kernel_mmap.entry_count].length = entry->length;
                    kernel_mmap.entries[kernel_mmap.entry_count].type = (mmap_type_t)entry->type;
                    kernel_mmap.entry_count++;
                }
                serial_printf("MULTIBOOT2: mregion: base=%llx, len=%llx, type=%d\n", entry->base_addr, entry->length, entry->type);
            }
            break;
        }
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
            multiboot_tag_framebuffer_t *framebuffer = (multiboot_tag_framebuffer_t *)tag;

            kernel_fb_info.fb_addr = framebuffer->framebuffer_addr;
            kernel_fb_info.fb_width = framebuffer->framebuffer_width;
            kernel_fb_info.fb_height = framebuffer->framebuffer_height;
            kernel_fb_info.fb_pitch = framebuffer->framebuffer_pitch;
            kernel_fb_info.fb_bpp = framebuffer->framebuffer_bpp;

            serial_printf(
                "MULTIBOOT2: framebuffer: %dx%dx%d at %llx, type=%d\n",
                framebuffer->framebuffer_width, framebuffer->framebuffer_height,
                framebuffer->framebuffer_bpp, framebuffer->framebuffer_addr,
                framebuffer->framebuffer_type);

            if (framebuffer->framebuffer_type == 2)
                panic();

            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
            multiboot_tag_old_acpi_t *old = (multiboot_tag_old_acpi_t *)tag;
            rsdp_phys_addr = (phys_addr_t)old->rsdp;
            memcpy(&rsdp_stable_copy, (void *)P2V(rsdp_phys_addr), 20);
            serial_printf("MULTIBOOT2: ACPI Old (v1.0) RSDP physical found at %llx\n", rsdp_phys_addr);
            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
            multiboot_tag_new_acpi_t *new = (multiboot_tag_new_acpi_t *)tag;
            rsdp_phys_addr = (phys_addr_t) new->rsdp;
            rsdp_t *temp_rsdp = (rsdp_t *)P2V(rsdp_phys_addr);
            size_t rsdp_size = (temp_rsdp->revision >= 2) ? temp_rsdp->length : 20;
            if (rsdp_size > sizeof(rsdp_t))
                rsdp_size = sizeof(rsdp_t);
            memcpy(&rsdp_stable_copy, temp_rsdp, rsdp_size);
            serial_printf("MULTIBOOT2: ACPI New (v2.0+) RSDP physical found at %llx\n", rsdp_phys_addr);
            break;
        }

        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
            multiboot_tag_basic_meminfo_t *meminfo = (multiboot_tag_basic_meminfo_t *)tag;

            serial_printf("MULTIBOOT2: mem_lower=%u KB, mem_upper=%u KB (%u MB)\n", meminfo->mem_lower, meminfo->mem_upper, meminfo->mem_upper / 1024);
            break;
        }

        case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR: {
            multiboot_tag_load_base_addr_t *load_base = (multiboot_tag_load_base_addr_t *)tag;
            uintptr_t actual_phys_addr = (uintptr_t)load_base->load_base_addr;
            uintptr_t expected_phys_addr = KERNEL_START_PHYS;

            serial_printf("MULTIBOOT2: load_base_addr=%lx (linked at %lx)\n", actual_phys_addr, expected_phys_addr);

            if (actual_phys_addr != expected_phys_addr) {
                intptr_t offset = (intptr_t)actual_phys_addr - (intptr_t)expected_phys_addr;
                serial_printf("MULTIBOOT2: ERROR: Kernel was relocated! Offset: %lx\n", offset);
                panic();
            }
            break;
        }

        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
        case MULTIBOOT_TAG_TYPE_EFI_BS:
        case MULTIBOOT_TAG_TYPE_EFI_64:
            break;

        default: {
            serial_printf("MULTIBOOT2: unknown type %d\n", tag->type);
            break;
        }
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7U) & ~0x7U));
    }
}

phys_addr_t get_rsdp_phys_addr() {
    return rsdp_phys_addr;
}
