/**
 * @file init.c
 * @brief Hauptinitialisierungsroutine und Multiboot2-Parser des Kernels.
 * @details Richtet die grundlegenden Architektur-Komponenten (GDT, IDT, PMM, VMM, Heap,
 *          ACPI, APIC, HPET, RTC), das Multi-Core-Scheduling und die ersten Init-Threads ein.
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/fpu.h>
#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/handler.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/cpu/irq.h>
#include <arch/x86_64/cpu/rtc.h>
#include <arch/x86_64/cpu/smp.h>
#include <arch/x86_64/drivers/pci.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/pmm.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/init.h>
#include <core/panic.h>
#include <core/process.h>
#include <core/scheduler.h>
#include <core/taskmgr.h>
#include <core/thread.h>
#include <lib/string.h>
#include <pch.h>

mmap_t kernel_mmap;
fb_info_t kernel_fb_info;
multiboot_info_t *kernel_multiboot_info;
char kernel_cmdline[256];
char kernel_bootloader_name[64];
boot_modules_t kernel_modules;
phys_addr_t rsdp_phys_addr = 0;
static rsdp_t rsdp_stable_copy;

/**
 * @brief Liest und verarbeitet die Multiboot2-Tags des Bootloaders.
 * @details Extrahiert Kernel-Befehlszeile, Bootloader-Namen, geladene Module,
 *          Speichermap, Framebuffer-Konfiguration und ACPI-RSDP-Zeiger.
 * @param magic Die von GRUB/Multiboot übergebene Magic Number.
 * @param info_ptr Physikalische Adresse der Multiboot-Tags.
 */
static void multiboot_parse(const uint64_t magic, const uint64_t info_ptr) {
    if (magic != MULTIBOOT2_MAGIC) {
        panic("bad multiboot2 magic", magic);
    } else if (!info_ptr) {
        panic("bad multiboot2 info_ptr", info_ptr);
    }

    kernel_multiboot_info = (multiboot_info_t *)info_ptr;

    uint32_t total_size = kernel_multiboot_info->total_size;
    uint8_t *end_addr = (uint8_t *)info_ptr + total_size;

    multiboot_tag_t *tag = (multiboot_tag_t *)(info_ptr + sizeof(multiboot_info_t));

    while ((uint8_t *)tag < end_addr && tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->size == 0) {
            serial_printf(COM1, "MULTIBOOT2: Corrupt tag with size 0\n");
            break;
        }

        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE: {
            multiboot_tag_cmdline_t *cmdline = (multiboot_tag_cmdline_t *)tag;
            strncpy(kernel_cmdline, cmdline->string, sizeof(kernel_cmdline) - 1);
            kernel_cmdline[sizeof(kernel_cmdline) - 1] = '\0';
            serial_printf(COM1, "MULTIBOOT2: cmdline=%s\n", kernel_cmdline);
            break;
        }
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER: {
            multiboot_tag_boot_loader_t *boot_loader = (multiboot_tag_boot_loader_t *)tag;
            strncpy(kernel_bootloader_name, boot_loader->string, sizeof(kernel_bootloader_name) - 1);
            kernel_bootloader_name[sizeof(kernel_bootloader_name) - 1] = '\0';
            serial_printf(COM1, "MULTIBOOT2: bootloader=%s\n", kernel_bootloader_name);
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
                serial_printf(COM1, "MULTIBOOT2: module at [%x - %x] cmd=%s\n", entry->mod_start, entry->mod_end, entry->cmdline);
            } else {
                serial_printf(COM1, "MULTIBOOT2: warning, max modules reached\n");
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
                serial_printf(COM1, "MULTIBOOT2: mregion: base=%llx, len=%llx, type=%d\n", entry->base_addr, entry->length, entry->type);
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
                COM1, "MULTIBOOT2: framebuffer: %dx%dx%d at %llx, type=%d\n",
                framebuffer->framebuffer_width, framebuffer->framebuffer_height,
                framebuffer->framebuffer_bpp, framebuffer->framebuffer_addr,
                framebuffer->framebuffer_type);

            if (framebuffer->framebuffer_type == 2)
                panic("Unsupported framebuffer type: EGA text mode is not supported", framebuffer->framebuffer_type);

            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
            multiboot_tag_old_acpi_t *old = (multiboot_tag_old_acpi_t *)tag;
            rsdp_phys_addr = (phys_addr_t)old->rsdp;
            memcpy(&rsdp_stable_copy, (void *)P2V(rsdp_phys_addr), 20);
            serial_printf(COM1, "MULTIBOOT2: ACPI Old (v1.0) RSDP physical found at %llx\n", rsdp_phys_addr);
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
            serial_printf(COM1, "MULTIBOOT2: ACPI New (v2.0+) RSDP physical found at %llx\n", rsdp_phys_addr);
            break;
        }

        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
            multiboot_tag_basic_meminfo_t *meminfo = (multiboot_tag_basic_meminfo_t *)tag;

            serial_printf(COM1, "MULTIBOOT2: mem_lower=%u KB, mem_upper=%u KB (%u MB)\n", meminfo->mem_lower, meminfo->mem_upper, meminfo->mem_upper / 1024);
            break;
        }

        case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR: {
            multiboot_tag_load_base_addr_t *load_base = (multiboot_tag_load_base_addr_t *)tag;
            uintptr_t actual_phys_addr = (uintptr_t)load_base->load_base_addr;
            uintptr_t expected_phys_addr = KERNEL_START_PHYS;

            serial_printf(COM1, "MULTIBOOT2: load_base_addr=%lx (linked at %lx)\n", actual_phys_addr, expected_phys_addr);

            if (actual_phys_addr != expected_phys_addr) {
                intptr_t offset = (intptr_t)actual_phys_addr - (intptr_t)expected_phys_addr;
                serial_printf(COM1, "MULTIBOOT2: ERROR: Kernel was relocated! Offset: %lx\n", offset);
                panic("Kernel relocation mismatch", (uint64_t)offset);
            }
            break;
        }

        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
        case MULTIBOOT_TAG_TYPE_EFI_BS:
        case MULTIBOOT_TAG_TYPE_EFI_64:
            break;

        default: {
            serial_printf(COM1, "MULTIBOOT2: unknown type %d\n", tag->type);
            break;
        }
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7U) & ~0x7U));
    }
}

/**
 * @brief Erster Kernel-Thread nach der Aktivierung des Schedulers.
 * @param arg Nicht verwendeter Argumentzeiger.
 */
static void kernel_init_thread(void *arg) {
    (void)arg;

    serial_printf(COM1, "INIT: kernel_init_thread started\n");

    // serial_printf(COM1, "\n==================== LIBM GENERIC API TEST ====================\n");

    // serial_printf(COM1, "[TRIG]  math_sin(90 deg)     = %f  (Erwartet: 1.000000)\n", math_sin(math_deg_to_rad(90.0)));
    // serial_printf(COM1, "[TRIG]  math_cos(180 deg)    = %f  (Erwartet: -1.000000)\n", math_cos(math_deg_to_rad(180.0)));
    // serial_printf(COM1, "[TRIG]  math_tan(45 deg)     = %f  (Erwartet: 1.000000)\n", math_tan(math_deg_to_rad(45.0)));
    // serial_printf(COM1, "[TRIG]  math_asin(1.0)       = %f  (Erwartet: %f rad)\n", math_asin(1.0), M_PI_2);
    // serial_printf(COM1, "[TRIG]  math_acos(1.0)       = %f  (Erwartet: 0.000000)\n", math_acos(1.0));
    // serial_printf(COM1, "[TRIG]  math_atan(1.0)       = %f  (Erwartet: %f rad)\n", math_atan(1.0), M_PI_4);
    // serial_printf(COM1, "[TRIG]  math_atan2(1.0, 1.0) = %f  (Erwartet: %f rad)\n", math_atan2(1.0, 1.0), M_PI_4);

    // double s_val = 0, c_val = 0;
    // math_sincos(math_deg_to_rad(90.0), &s_val, &c_val);
    // serial_printf(COM1, "[TRIG]  math_sincos(90 deg)  = Sin: %f, Cos: %f\n", s_val, c_val);

    // serial_printf(COM1, "[HYP]   math_sinh(0.0)       = %f  (Erwartet: 0.000000)\n", math_sinh(0.0));
    // serial_printf(COM1, "[HYP]   math_cosh(0.0)       = %f  (Erwartet: 1.000000)\n", math_cosh(0.0));
    // serial_printf(COM1, "[HYP]   math_tanh(0.0)       = %f  (Erwartet: 0.000000)\n", math_tanh(0.0));
    // serial_printf(COM1, "[HYP]   math_asinh(0.0)      = %f  (Erwartet: 0.000000)\n", math_asinh(0.0));
    // serial_printf(COM1, "[HYP]   math_acosh(1.0)      = %f  (Erwartet: 0.000000)\n", math_acosh(1.0));
    // serial_printf(COM1, "[HYP]   math_atanh(0.0)      = %f  (Erwartet: 0.000000)\n", math_atanh(0.0));

    // serial_printf(COM1, "[EXP]   math_exp(1.0)        = %f  (Erwartet: %f [e])\n", math_exp(1.0), M_E);
    // serial_printf(COM1, "[EXP]   math_exp2(3.0)       = %f  (Erwartet: 8.000000)\n", math_exp2(3.0));
    // serial_printf(COM1, "[EXP]   math_exp10(2.0)      = %f  (Erwartet: 100.000000)\n", math_exp10(2.0));
    // serial_printf(COM1, "[EXP]   math_expm1(0.0)      = %f  (Erwartet: 0.000000)\n", math_expm1(0.0));
    // serial_printf(COM1, "[LOG]   math_log(2.71828...) = %f  (Erwartet: 1.000000)\n", math_log(M_E));
    // serial_printf(COM1, "[LOG]   math_log2(1024.0)    = %f  (Erwartet: 10.000000)\n", math_log2(1024.0));
    // serial_printf(COM1, "[LOG]   math_log10(1000.0)   = %f  (Erwartet: 3.000000)\n", math_log10(1000.0));
    // serial_printf(COM1, "[LOG]   math_log1p(0.0)      = %f  (Erwartet: 0.000000)\n", math_log1p(0.0));
    // serial_printf(COM1, "[LOG]   math_logb(1024.0)    = %f  (Erwartet: 10.000000)\n", math_logb(1024.0));

    // serial_printf(COM1, "[POW]   math_pow(2.0, 10.0)  = %f  (Erwartet: 1024.000000)\n", math_pow(2.0, 10.0));
    // serial_printf(COM1, "[POW]   math_sqrt(16.0)      = %f  (Erwartet: 4.000000)\n", math_sqrt(16.0));
    // serial_printf(COM1, "[POW]   math_cbrt(27.0)      = %f  (Erwartet: 3.000000)\n", math_cbrt(27.0));
    // serial_printf(COM1, "[POW]   math_hypot(3.0, 4.0) = %f  (Erwartet: 5.000000)\n", math_hypot(3.0, 4.0));
    // serial_printf(COM1, "[NUM]   math_fabs(-13.37)    = %f  (Erwartet: 13.370000)\n", (double)math_fabs((float)-13.37));

    // serial_printf(COM1, "[RND]   math_ceil(4.2)       = %f  (Erwartet: 5.000000)\n", math_ceil(4.2));
    // serial_printf(COM1, "[RND]   math_floor(4.7)      = %f  (Erwartet: 4.000000)\n", math_floor(4.7));
    // serial_printf(COM1, "[RND]   math_trunc(4.9)      = %f  (Erwartet: 4.000000)\n", math_trunc(4.9));
    // serial_printf(COM1, "[RND]   math_round(4.5)      = %f  (Erwartet: 5.000000)\n", math_round(4.5));
    // serial_printf(COM1, "[RND]   math_rint(4.5)       = %f  (Abhängig von FPU Rounding-Mode)\n", math_rint(4.5));
    // serial_printf(COM1, "[RND]   math_nearbyint(4.5)  = %f  (Abhängig von FPU Rounding-Mode)\n", math_nearbyint(4.5));
    // serial_printf(COM1, "[MOD]   math_fmod(5.3, 2.0)  = %f  (Erwartet: 1.300000)\n", math_fmod(5.3, 2.0));
    // serial_printf(COM1, "[MOD]   math_remainder(5.3, 2.0) = %f (Erwartet: -0.700000)\n", math_remainder(5.3, 2.0));

    // serial_printf(COM1, "[MAN]   math_copysign(5.0, -1.0) = %f (Erwartet: -5.000000)\n", math_copysign(5.0, -1.0));
    // serial_printf(COM1, "[MAN]   math_nextafter(1.0, 2.0) = %.15f (Nächste Floating-Point-Zahl)\n", math_nextafter(1.0, 2.0));
    // serial_printf(COM1, "[MIN]   math_fmax(12.0, 42.5)    = %f  (Erwartet: 42.500000)\n", math_fmax(12.0, 42.5));
    // serial_printf(COM1, "[MIN]   math_fmin(12.0, 42.5)    = %f  (Erwartet: 12.000000)\n", math_fmin(12.0, 42.5));
    // serial_printf(COM1, "[MIN]   math_fdim(42.0, 12.0)    = %f  (Erwartet: 30.000000)\n", math_fdim(42.0, 12.0));
    // serial_printf(COM1, "[MAN]   math_fma(2.0, 3.0, 4.0)  = %f  (Erwartet: 10.000000)\n", math_fma(2.0, 3.0, 4.0));

    // serial_printf(COM1, "[SPEC]  math_erf(0.0)       = %f  (Erwartet: 0.000000)\n", math_erf(0.0));
    // serial_printf(COM1, "[SPEC]  math_erfc(0.0)      = %f  (Erwartet: 1.000000)\n", math_erfc(0.0));
    // serial_printf(COM1, "[SPEC]  math_lgamma(1.0)    = %f  (Erwartet: 0.000000)\n", math_lgamma(1.0));
    // serial_printf(COM1, "[SPEC]  math_tgamma(5.0)    = %f  (Erwartet: 24.000000 [4!])\n", math_tgamma(5.0));

    // float f_deg = 180.0f;
    // double d_deg = 180.0;
    // long double ld_deg = 180.0L;
    // serial_printf(COM1, "[TYPE]  math_deg_to_rad(float)       = %f\n", (double)math_deg_to_rad(f_deg));
    // serial_printf(COM1, "[TYPE]  math_deg_to_rad(double)      = %f\n", math_deg_to_rad(d_deg));
    // serial_printf(COM1, "[TYPE]  math_deg_to_rad(long double) = %Lf\n", math_deg_to_rad(ld_deg));

    // serial_printf(COM1, "===============================================================\n\n");

    while (1) {
        thread_yield();
    }
}

void kernel_init(const uint64_t magic, const uint64_t info_ptr) {
    serial_init(COM1);
    serial_printf(COM1, "INIT: start\n");

    gdt_init();
    idt_init();
    irq_init();

    cpu_fpu_init();

    idt_enable();

    serial_printf(COM1, "MULTIBOOT2: magic=%x\n", magic);
    serial_printf(COM1, "MULTIBOOT2: info_ptr=%x\n", info_ptr);

    multiboot_parse(magic, info_ptr);

    pmm_init();
    vmm_init();
    heap_init();

    if (rsdp_phys_addr == 0) {
        serial_printf(COM1, "ACPI: no RSDP address found in the MULTIBOOT2 info structure\n");
        panic("Kernel can't start without ACPI", 0);
    }

    acpi_init(rsdp_phys_addr);

    apic_init();
    hpet_init();
    rtc_init();

    pci_init();

    lapic_timer_calibrate_and_start(1000);

    scheduler_init();

    smp_init();

    serial_printf(COM1, "INIT: Multi-Core Scheduling running!\n");

    thread_create_on_cpu(kernel_process, kernel_init_thread, NULL, "kernel_init", -1);

    scheduler_enable();
    scheduler_thread_exit();

    // Unerreichbarer Code (Fallback)

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
