/**
 * @file multiboot2.c
 * @brief Multiboot2 parsing.
 * @author friedrichOsDev
 */

#include "../acpi/acpi.h"
#include "../mm/mem_def.h"
#include "_multiboot2.h"
#include <arch/arch.h>
#include <arch/multiboot2.h>
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
static bool rsdp_copied = false;
static multiboot_info_t *kernel_multiboot_info;

/**
 * @brief Parse the Multiboot2 header.
 *
 * @param magic The Multiboot2 magic number.
 * @param info_ptr The address of the Multiboot2 header.
 */
void multiboot_parse(const uint64_t magic, const uint64_t info_ptr) {
    k_assert(magic == MULTIBOOT2_MAGIC);
    k_assert(info_ptr != 0);

    kernel_multiboot_info = (multiboot_info_t *)info_ptr;

    uint32_t total_size = kernel_multiboot_info->total_size;
    uint8_t *end_addr = (uint8_t *)info_ptr + total_size;

    multiboot_tag_t *tag = (multiboot_tag_t *)(info_ptr + sizeof(multiboot_info_t));

    while ((uint8_t *)tag < end_addr && tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->size == 0) {
            log_warn("corrupt multiboot2 tag with size 0");
            break;
        }

        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE: {
            multiboot_tag_cmdline_t *cmdline = (multiboot_tag_cmdline_t *)tag;
            strncpy(kernel_cmdline, cmdline->string, sizeof(kernel_cmdline) - 1);
            kernel_cmdline[sizeof(kernel_cmdline) - 1] = '\0';
            log_debug("multiboot2 cmdline=%s", kernel_cmdline);
            break;
        }
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER: {
            multiboot_tag_boot_loader_t *boot_loader = (multiboot_tag_boot_loader_t *)tag;
            strncpy(kernel_bootloader_name, boot_loader->string, sizeof(kernel_bootloader_name) - 1);
            kernel_bootloader_name[sizeof(kernel_bootloader_name) - 1] = '\0';
            log_debug("multiboot2 bootloader=%s", kernel_bootloader_name);
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
                log_debug("multiboot2 module at [%x - %x] cmd=%s", entry->mod_start, entry->mod_end, entry->cmdline);
            } else {
                log_warn("multiboot2 max modules reached");
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
                log_debug("multiboot2 memory region: base=%llx, len=%llx, type=%d", entry->base_addr, entry->length, entry->type);
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

            log_debug("multiboot2 framebuffer: %dx%dx%d at %llx, type=%d", framebuffer->framebuffer_width, framebuffer->framebuffer_height, framebuffer->framebuffer_bpp, framebuffer->framebuffer_addr, framebuffer->framebuffer_type);
            k_assert(framebuffer->framebuffer_type != 2);
            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
            multiboot_tag_old_acpi_t *old = (multiboot_tag_old_acpi_t *)tag;
            rsdp_phys_addr = (phys_addr_t)old->rsdp;
            memcpy(&rsdp_stable_copy, (void *)P2V(rsdp_phys_addr), 20);
            rsdp_copied = true;
            log_debug("multiboot2 ACPI old (v1.0) RSDP physical found at %llx", rsdp_phys_addr);
            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
            multiboot_tag_new_acpi_t *new = (multiboot_tag_new_acpi_t *)tag;
            rsdp_phys_addr = (phys_addr_t)new->rsdp;
            rsdp_t *temp_rsdp = (rsdp_t *)P2V(rsdp_phys_addr);
            size_t rsdp_size = (temp_rsdp->revision >= 2) ? temp_rsdp->length : 20;
            if (rsdp_size > sizeof(rsdp_t))
                rsdp_size = sizeof(rsdp_t);
            memcpy(&rsdp_stable_copy, temp_rsdp, rsdp_size);
            rsdp_copied = true;
            log_debug("multiboot2 ACPI new (v2.0+) RSDP physical found at %llx", rsdp_phys_addr);
            break;
        }

        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
            multiboot_tag_basic_meminfo_t *meminfo = (multiboot_tag_basic_meminfo_t *)tag;

            log_debug("multiboot2 mem_lower=%u KB, mem_upper=%u KB (%u MB)", meminfo->mem_lower, meminfo->mem_upper, meminfo->mem_upper / 1024);
            break;
        }

        case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR: {
            multiboot_tag_load_base_addr_t *load_base = (multiboot_tag_load_base_addr_t *)tag;
            uintptr_t actual_phys_addr = (uintptr_t)load_base->load_base_addr;
            uintptr_t expected_phys_addr = KERNEL_START_PHYS;

            log_debug("multiboot2 load_base_addr=%lx (linked at %lx)", actual_phys_addr, expected_phys_addr);

            if (actual_phys_addr != expected_phys_addr) {
                intptr_t offset = (intptr_t)actual_phys_addr - (intptr_t)expected_phys_addr;
                log_panic("kernel was relocated: offset=%lx", offset);
            }
            break;
        }

        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
        case MULTIBOOT_TAG_TYPE_EFI_BS:
        case MULTIBOOT_TAG_TYPE_EFI_64:
            break;

        default: {
            log_warn("unknown multiboot2 type %d", tag->type);
            break;
        }
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7U) & ~0x7U));
    }
}

/**
 * @brief Get the physical address of the RSDP (Root System Description Pointer).
 *
 * @return phys_addr_t The physical address of the RSDP.
 */
virt_addr_t get_rsdp_addr() {
    if (!rsdp_copied) return 0;
    return (virt_addr_t)&rsdp_stable_copy;
}
