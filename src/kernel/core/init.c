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
#include <shell/shell.h>

/**
 * @brief Polls HID events on a dedicated kernel thread.
 *
 * The thread drains pending events in bounded batches and sleeps briefly when
 * the queue is empty, avoiding a busy polling loop while keeping input latency
 * low. Input processing is isolated from driver and graphics initialization.
 *
 * @param[in] arg Unused argument.
 */
static void kernel_input_thread(void *arg) {
    (void)arg;

    for (;;) {
        size_t processed = 0U;

        while (processed < 64U) {
            input_event_t event;
            kerr_t err = input_poll_event(&event);

            if (err == E_NOTAVAIL) {
                break;
            }
            if (k_failed(err)) {
                log_warn("input event polling failed (%s)", kerr_str(err));
                break;
            }

            processed++;

            if (event.hid.type == HID_EVENT_KEY_PRESS &&
                event.hid.key.keycode == KEY_ESC &&
                event.key_action == INPUT_KEY_ACTION_PRESSED) {
                log_info("ESC pressed, shutting down");
                acpi_power_off();
                log_warn("ACPI shutdown request returned");
            }

            shell_handle_event(&event);
        }

        if (processed < 64U) {
            /*
             * The queue was drained (or polling failed). Sleep for one tick
             * instead of immediately polling again and consuming a full CPU.
             */
            thread_sleep_ms(1U);
        } else {
            /* Bound each batch so rendering and other kernel threads progress. */
            thread_yield();
        }
    }
}

/**
 * @brief Initializes kernel drivers and starts shell and input workers.
 *
 * @param[in] arg Unused argument.
 */
static void kernel_init_thread(void *arg) {
    (void)arg;

    kerr_t err = gfx_init();
    bool graphics_ready = !k_failed(err);
    if (!graphics_ready) {
        log_warn("gfx init failed, no graphical output available (%s)", kerr_str(err));
    }
    log_info("gfx initialization finished");

    err = hid_init();
    if (k_failed(err)) {
        log_warn("hid initialization failed (%s)", kerr_str(err));
        thread_exit();
    }
    log_info("hid done");

    err = input_init();
    if (k_failed(err)) {
        log_warn("input initialization failed (%s)", kerr_str(err));
        thread_exit();
    }

    err = ps2_register_driver();
    if (k_failed(err)) {
        log_warn("PS/2 driver unavailable (%s)", kerr_str(err));
    }
    log_info("input initialization done");

    if (graphics_ready) {
        version_splash_show("0.0.3", 1000);
        err = shell_init();
        if (k_failed(err)) {
            log_warn("shell initialization failed (%s)", kerr_str(err));
        }
    } else {
        log_warn("kernel shell disabled because graphics are unavailable");
    }

    thread_t *input_thread = NULL;
    err = thread_create(proc_get_kernel_proc(), kernel_input_thread, NULL,
                        "kernel input", &input_thread);
    if (k_failed(err)) {
        log_error("failed to create kernel input thread (%s)", kerr_str(err));
        thread_exit();
    }

    log_info("kernel input thread started");
    thread_exit();
}

/**
 * @brief Initialize the kernel and start the scheduler.
 *
 * @param[in] magic Multiboot2 magic number.
 * @param[in] info_ptr Pointer to the multiboot2 information structure.
 */
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
