/**
 * @file init.c
 * @brief Kernel initialization
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <arch/multiboot2.h>
#include <drivers/gfx/compositor.h>
#include <drivers/gfx/gfx.h>
#include <drivers/gfx/version_splash.h>
#include <drivers/hid/hid.h>
#include <drivers/hid/input.h>
#ifdef KERNEL_DEBUG_BUILD
#include <drivers/hid/hid_test.h>
#endif
#include <drivers/hid/ps2/ps2.h>
#include <drivers/pci.h>
#include <drivers/rtc.h>
#include <kernel/panic.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <pch.h>
#include <sched/process.h>
#include <sched/sched.h>
#include <sched/taskmgr.h>
#include <sched/thread.h>

static void kernel_init_thread(void *arg) {
    (void)arg;

    kerr_t err = gfx_init();
    if (k_failed(err)) {
        log_warn("gfx init failed, no graphical output available (%s)", kerr_str(err));
    }
    log_info("gfx done");

    err = hid_init();
    if (k_failed(err)) {
        log_warn("hid init failed, no keyboard or mouse will be available (%s)", kerr_str(err));
    } else {
        log_info("hid done");
        err = input_init();
        if (k_failed(err)) {
            log_warn("input API initialization failed (%s)", kerr_str(err));
        } else {
            err = ps2_register_driver();
            if (k_failed(err)) log_warn("PS/2 driver unavailable (%s)", kerr_str(err));
        }
    }
    log_info("input initialization done");

    version_splash_show("0.0.3", 3000);

    while (1) {
#ifdef KERNEL_DEBUG_BUILD
        (void)hid_test_poll(32);
#endif
        thread_yield();
    }
}

void kernel_init(const uint64_t magic, const uint64_t info_ptr); // first declaration
void kernel_init(const uint64_t magic, const uint64_t info_ptr) {
    serial_init();
    log_info("start");

    gdt_init();
    log_info("gdt done");
    idt_init();
    log_info("idt done");
    irq_init();
    log_info("irq done");
    fpu_init();
    log_info("fpu done");

    arch_sti();
    log_info("interrupts enabled");

    multiboot_parse(magic, info_ptr);
    log_info("multiboot2 done");

    K_TRY_PANIC(pmm_init());
    log_info("pmm done");
    K_TRY_PANIC(vmm_init());
    log_info("vmm done");
    K_TRY_PANIC(heap_init());
    log_info("heap done");

    virt_addr_t rsdp_addr = get_rsdp_addr();
    k_assert(rsdp_addr != 0);
    K_TRY_PANIC(acpi_init(rsdp_addr));
    log_info("acpi done");

    K_TRY_PANIC(apic_init());
    log_info("apic done");
    hpet_init();
    log_info("hpet done");
    rtc_init();
    log_info("rtc done");

    K_TRY_PANIC(pci_init());
    log_info("pci done");

    log_debug("lapic_target_hz=%d", 1000);
    lapic_init(1000);
    log_info("lapic done");

    K_TRY_PANIC(sched_init());
    log_info("sched done");

    K_TRY_PANIC(smp_init());
    log_info("smp done");

    log_info("multi-core scheduling running");

    thread_t *kernel_init_th = NULL;
    K_TRY_PANIC(thread_create_on_cpu(proc_get_kernel_proc(), kernel_init_thread, NULL, "kernel_init", -1, &kernel_init_th));
    log_debug("kernel_init_thread created");

    sched_enable();
    log_info("sched enabled");

    thread_exit();

    log_panic("no code schould be executed after thread_exit()");

    while (1) {
        arch_hlt();
    }
}
