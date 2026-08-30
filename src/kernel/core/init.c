/**
 * @file init.c
 * @brief Kernel initialization code
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/handler.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/cpu/irq.h>
#include <arch/x86_64/cpu/smp.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/pmm.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/init.h>
#include <core/panic.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <lib/string.h>

mmap_t kernel_mmap;
fb_info_t kernel_fb_info;
multiboot_info_t *kernel_multiboot_info;
char kernel_cmdline[256];
char kernel_bootloader_name[64];
phys_addr_t rsdp_phys_addr = 0;
static rsdp_t rsdp_stable_copy;

/**
 * Parses the MULTIBOOT2 info
 * @param magic The MULTIBOOT2 magic number
 * @param info_ptr The MULTIBOOT2 info address
 */
static void multiboot_parse(const uint64_t magic, const uint64_t info_ptr) {
    if (magic != MULTIBOOT2_MAGIC) {
        panic("bad multiboot2 magic", magic);
    } else if (!info_ptr) {
        panic("bad multiboot2 info_ptr", info_ptr);
    }

    kernel_multiboot_info = (multiboot_info_t *)info_ptr;
    multiboot_tag_t *tag =
        (multiboot_tag_t *)(info_ptr + sizeof(multiboot_info_t));

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        serial_printf(COM1, "MULTIBOOT2: type=%x, size=%x\n", tag->type,
                      tag->size);
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE: {
            multiboot_tag_cmdline_t *cmdline = (multiboot_tag_cmdline_t *)tag;
            if (cmdline->size > 8 && cmdline->string[0] != '\0') {
                uint32_t len = cmdline->size - 8;
                if (len > sizeof(kernel_cmdline) - 1)
                    len = sizeof(kernel_cmdline) - 1;
                for (uint32_t i = 0; i < len; i++)
                    kernel_cmdline[i] = cmdline->string[i];
                kernel_cmdline[len] = '\0';
                serial_printf(COM1, "MULTIBOOT2: cmdline=%s\n", kernel_cmdline);
            } else {
                serial_printf(COM1, "MULTIBOOT2: cmdline=NULL\n");
            }
            break;
        }
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER: {
            multiboot_tag_boot_loader_t *boot_loader =
                (multiboot_tag_boot_loader_t *)tag;
            if (boot_loader->size > 8 && boot_loader->string[0] != '\0') {
                uint32_t len = boot_loader->size - 8;
                if (len > sizeof(kernel_bootloader_name) - 1)
                    len = sizeof(kernel_bootloader_name) - 1;
                for (uint32_t i = 0; i < len; i++)
                    kernel_bootloader_name[i] = boot_loader->string[i];
                kernel_bootloader_name[len] = '\0';
                serial_printf(COM1, "MULTIBOOT2: bootloader=%s\n",
                              kernel_bootloader_name);
            } else {
                serial_printf(COM1, "MULTIBOOT2: bootloader=NULL\n");
            }
            break;
        }
        case MULTIBOOT_TAG_TYPE_MODULE: {
            multiboot_tag_module_t *module = (multiboot_tag_module_t *)tag;
            (void)module;
            serial_printf(COM1, "MULTIBOOT2: module isn't implemented yet\n");
            break;
        }
        case MULTIBOOT_TAG_TYPE_MMAP: {
            multiboot_tag_mmap_t *mmap = (multiboot_tag_mmap_t *)tag;
            kernel_mmap.entry_count = 0;
            for (multiboot_tag_mmap_entry_t *entry = mmap->entries;
                 (uint8_t *)entry < (uint8_t *)tag + tag->size;
                 entry = (multiboot_tag_mmap_entry_t *)((uintptr_t)entry +
                                                        mmap->entry_size)) {
                if (kernel_mmap.entry_count < MMAP_MAX_ENTRIES) {
                    kernel_mmap.entries[kernel_mmap.entry_count].base_addr =
                        entry->base_addr;
                    kernel_mmap.entries[kernel_mmap.entry_count].length =
                        entry->length;
                    kernel_mmap.entries[kernel_mmap.entry_count].type =
                        (mmap_type_t)entry->type;
                    kernel_mmap.entry_count++;
                }
                serial_printf(
                    COM1, "MULTIBOOT2: mregion: base=%llx, len=%llx, type=%d\n",
                    entry->base_addr, entry->length, entry->type);
            }
            break;
        }
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
            multiboot_tag_framebuffer_t *framebuffer =
                (multiboot_tag_framebuffer_t *)tag;
            kernel_fb_info.fb_addr = framebuffer->framebuffer_addr;
            kernel_fb_info.fb_width = framebuffer->framebuffer_width;
            kernel_fb_info.fb_height = framebuffer->framebuffer_height;
            kernel_fb_info.fb_pitch = framebuffer->framebuffer_pitch;
            kernel_fb_info.fb_bpp = framebuffer->framebuffer_bpp;
            serial_printf(
                COM1, "MULTIBOOT2: framebuffer: %dx%dx%d at %llx, type=%d\n",
                framebuffer->framebuffer_width, framebuffer->framebuffer_height,
                framebuffer->framebuffer_bpp, framebuffer->framebuffer_addr,
                framebuffer->framebuffer_type);
            if (framebuffer->framebuffer_type == 2)
                panic("Unsupported framebuffer type: EGA text mode is not "
                      "supported",
                      framebuffer->framebuffer_type);
            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
            multiboot_tag_old_acpi_t *old = (multiboot_tag_old_acpi_t *)tag;
            rsdp_phys_addr = (phys_addr_t)old->rsdp;
            memcpy(&rsdp_stable_copy, (void *)P2V(rsdp_phys_addr), 20);
            serial_printf(
                COM1,
                "MULTIBOOT2: ACPI Old (v1.0) RSDP physical found at %llx\n",
                rsdp_phys_addr);
            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
            multiboot_tag_new_acpi_t *new = (multiboot_tag_new_acpi_t *)tag;
            rsdp_phys_addr = (phys_addr_t) new->rsdp;
            rsdp_t *temp_rsdp = (rsdp_t *)P2V(rsdp_phys_addr);
            size_t rsdp_size =
                (temp_rsdp->revision >= 2) ? temp_rsdp->length : 20;
            if (rsdp_size > sizeof(rsdp_t))
                rsdp_size = sizeof(rsdp_t);
            memcpy(&rsdp_stable_copy, temp_rsdp, rsdp_size);
            serial_printf(
                COM1,
                "MULTIBOOT2: ACPI New (v2.0+) RSDP physical found at %llx\n",
                rsdp_phys_addr);
            break;
        }
        default: {
            serial_printf(COM1, "MULTIBOOT2: unknown type %d\n", tag->type);
            break;
        }
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~0x7));
    }
}

static void timer_callback(struct registers *regs) {
    (void)regs;
    scheduler_tick();

    if (smp_cpu_count > 1) {
        lapic_send_broadcast_tick_ipi();
    }
}

void core_worker(void *arg) {
    const char *task_name = (const char *)arg;
    cpu_local_t *cpu = smp_get_current_cpu();
    thread_t *curr = scheduler_get_current_thread();

    uint64_t counter = 0;
    while (1) {
        counter++;
        if (counter % 100000000 == 0) {
            cpu = smp_get_current_cpu();
            curr = scheduler_get_current_thread();
            serial_printf(
                COM1, "[CPU %d | TID %llu] Task '%s' calculating (cnt=%llu)\n",
                cpu ? cpu->cpu_id : 0, curr ? curr->tid : 0, task_name,
                counter);
        }
    }
}

/**
 * Initializes the Kernel
 * @param magic The MULTIBOOT2 magic number given by GRUB
 * @param info_ptr The MULTIBOOT2 info address given by GRUB
 */
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
    heap_init();

    if (rsdp_phys_addr == 0) {
        serial_printf(
            COM1,
            "ACPI: no RSDP address found in the MULTIBOOT2 info structure\n");
        panic("Kernel can't start without ACPI", 0);
    }

    acpi_init(rsdp_phys_addr);

    apic_init();
    hpet_init();

    ioapic_route_irq(0, 32, 0); // Route IRQ 0 to vector 32 on BSP (APIC ID 0)
    // irq_install_handler(0, timer_callback); <- timer_callback would trigger
    // every ms

    scheduler_init();
    smp_init();

    irq_install_handler(0, timer_callback);

    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_0", "worker_c0", 0);
    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_1", "worker_c1", 1);
    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_2", "worker_c2", 2);
    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_3", "worker_c3", 3);
    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_4", "worker_c4", 4);
    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_5", "worker_c5", 5);
    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_6", "worker_c6", 6);
    thread_create_on_cpu(NULL, core_worker, "PINNED_TO_CORE_7", "worker_c7", 7);
    thread_create(NULL, core_worker, "FLOATING_TASK_0", "floating_0");
    thread_create(NULL, core_worker, "FLOATING_TASK_1", "floating_1");

    serial_printf(COM1, "INIT: Multi-Core Scheduling running!\n");

    idt_enable(); // just to be sure it's enabled

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
