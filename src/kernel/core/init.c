/**
 * @file init.c
 * @brief Hauptinitialisierungsroutine und Multiboot2-Parser des Kernels.
 * @details Richtet die grundlegenden Architektur-Komponenten (GDT, IDT, PMM, VMM, Heap,
 *          ACPI, APIC, HPET, RTC), das Multi-Core-Scheduling und die ersten Init-Threads ein.
 * @author friedrichOsDev
 */

#include <arch/arch.h>

#include <boot/multiboot2.h>
#include <core/init.h>
#include <core/process.h>
#include <core/scheduler.h>
#include <core/taskmgr.h>
#include <core/thread.h>
#include <drivers/pci.h>
#include <drivers/rtc.h>
#include <kernel/error.h>
#include <kernel/panic.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <pch.h>

/**
 * @brief Erster Kernel-Thread nach der Aktivierung des Schedulers.
 * @param arg Nicht verwendeter Argumentzeiger.
 */
static void kernel_init_thread(void *arg) {
    (void)arg;

    serial_printf("INIT: kernel_init_thread started\n");

    // serial_printf("\n==================== LIBM GENERIC API TEST ====================\n");

    // serial_printf("[TRIG]  math_sin(90 deg)     = %f  (Erwartet: 1.000000)\n", math_sin(math_deg_to_rad(90.0)));
    // serial_printf("[TRIG]  math_cos(180 deg)    = %f  (Erwartet: -1.000000)\n", math_cos(math_deg_to_rad(180.0)));
    // serial_printf("[TRIG]  math_tan(45 deg)     = %f  (Erwartet: 1.000000)\n", math_tan(math_deg_to_rad(45.0)));
    // serial_printf("[TRIG]  math_asin(1.0)       = %f  (Erwartet: %f rad)\n", math_asin(1.0), M_PI_2);
    // serial_printf("[TRIG]  math_acos(1.0)       = %f  (Erwartet: 0.000000)\n", math_acos(1.0));
    // serial_printf("[TRIG]  math_atan(1.0)       = %f  (Erwartet: %f rad)\n", math_atan(1.0), M_PI_4);
    // serial_printf("[TRIG]  math_atan2(1.0, 1.0) = %f  (Erwartet: %f rad)\n", math_atan2(1.0, 1.0), M_PI_4);

    // double s_val = 0, c_val = 0;
    // math_sincos(math_deg_to_rad(90.0), &s_val, &c_val);
    // serial_printf("[TRIG]  math_sincos(90 deg)  = Sin: %f, Cos: %f\n", s_val, c_val);

    // serial_printf("[HYP]   math_sinh(0.0)       = %f  (Erwartet: 0.000000)\n", math_sinh(0.0));
    // serial_printf("[HYP]   math_cosh(0.0)       = %f  (Erwartet: 1.000000)\n", math_cosh(0.0));
    // serial_printf("[HYP]   math_tanh(0.0)       = %f  (Erwartet: 0.000000)\n", math_tanh(0.0));
    // serial_printf("[HYP]   math_asinh(0.0)      = %f  (Erwartet: 0.000000)\n", math_asinh(0.0));
    // serial_printf("[HYP]   math_acosh(1.0)      = %f  (Erwartet: 0.000000)\n", math_acosh(1.0));
    // serial_printf("[HYP]   math_atanh(0.0)      = %f  (Erwartet: 0.000000)\n", math_atanh(0.0));

    // serial_printf("[EXP]   math_exp(1.0)        = %f  (Erwartet: %f [e])\n", math_exp(1.0), M_E);
    // serial_printf("[EXP]   math_exp2(3.0)       = %f  (Erwartet: 8.000000)\n", math_exp2(3.0));
    // serial_printf("[EXP]   math_exp10(2.0)      = %f  (Erwartet: 100.000000)\n", math_exp10(2.0));
    // serial_printf("[EXP]   math_expm1(0.0)      = %f  (Erwartet: 0.000000)\n", math_expm1(0.0));
    // serial_printf("[LOG]   math_log(2.71828...) = %f  (Erwartet: 1.000000)\n", math_log(M_E));
    // serial_printf("[LOG]   math_log2(1024.0)    = %f  (Erwartet: 10.000000)\n", math_log2(1024.0));
    // serial_printf("[LOG]   math_log10(1000.0)   = %f  (Erwartet: 3.000000)\n", math_log10(1000.0));
    // serial_printf("[LOG]   math_log1p(0.0)      = %f  (Erwartet: 0.000000)\n", math_log1p(0.0));
    // serial_printf("[LOG]   math_logb(1024.0)    = %f  (Erwartet: 10.000000)\n", math_logb(1024.0));

    // serial_printf("[POW]   math_pow(2.0, 10.0)  = %f  (Erwartet: 1024.000000)\n", math_pow(2.0, 10.0));
    // serial_printf("[POW]   math_sqrt(16.0)      = %f  (Erwartet: 4.000000)\n", math_sqrt(16.0));
    // serial_printf("[POW]   math_cbrt(27.0)      = %f  (Erwartet: 3.000000)\n", math_cbrt(27.0));
    // serial_printf("[POW]   math_hypot(3.0, 4.0) = %f  (Erwartet: 5.000000)\n", math_hypot(3.0, 4.0));
    // serial_printf("[NUM]   math_fabs(-13.37)    = %f  (Erwartet: 13.370000)\n", (double)math_fabs((float)-13.37));

    // serial_printf("[RND]   math_ceil(4.2)       = %f  (Erwartet: 5.000000)\n", math_ceil(4.2));
    // serial_printf("[RND]   math_floor(4.7)      = %f  (Erwartet: 4.000000)\n", math_floor(4.7));
    // serial_printf("[RND]   math_trunc(4.9)      = %f  (Erwartet: 4.000000)\n", math_trunc(4.9));
    // serial_printf("[RND]   math_round(4.5)      = %f  (Erwartet: 5.000000)\n", math_round(4.5));
    // serial_printf("[RND]   math_rint(4.5)       = %f  (Abhängig von FPU Rounding-Mode)\n", math_rint(4.5));
    // serial_printf("[RND]   math_nearbyint(4.5)  = %f  (Abhängig von FPU Rounding-Mode)\n", math_nearbyint(4.5));
    // serial_printf("[MOD]   math_fmod(5.3, 2.0)  = %f  (Erwartet: 1.300000)\n", math_fmod(5.3, 2.0));
    // serial_printf("[MOD]   math_remainder(5.3, 2.0) = %f (Erwartet: -0.700000)\n", math_remainder(5.3, 2.0));

    // serial_printf("[MAN]   math_copysign(5.0, -1.0) = %f (Erwartet: -5.000000)\n", math_copysign(5.0, -1.0));
    // serial_printf("[MAN]   math_nextafter(1.0, 2.0) = %.15f (Nächste Floating-Point-Zahl)\n", math_nextafter(1.0, 2.0));
    // serial_printf("[MIN]   math_fmax(12.0, 42.5)    = %f  (Erwartet: 42.500000)\n", math_fmax(12.0, 42.5));
    // serial_printf("[MIN]   math_fmin(12.0, 42.5)    = %f  (Erwartet: 12.000000)\n", math_fmin(12.0, 42.5));
    // serial_printf("[MIN]   math_fdim(42.0, 12.0)    = %f  (Erwartet: 30.000000)\n", math_fdim(42.0, 12.0));
    // serial_printf("[MAN]   math_fma(2.0, 3.0, 4.0)  = %f  (Erwartet: 10.000000)\n", math_fma(2.0, 3.0, 4.0));

    // serial_printf("[SPEC]  math_erf(0.0)       = %f  (Erwartet: 0.000000)\n", math_erf(0.0));
    // serial_printf("[SPEC]  math_erfc(0.0)      = %f  (Erwartet: 1.000000)\n", math_erfc(0.0));
    // serial_printf("[SPEC]  math_lgamma(1.0)    = %f  (Erwartet: 0.000000)\n", math_lgamma(1.0));
    // serial_printf("[SPEC]  math_tgamma(5.0)    = %f  (Erwartet: 24.000000 [4!])\n", math_tgamma(5.0));

    // float f_deg = 180.0f;
    // double d_deg = 180.0;
    // long double ld_deg = 180.0L;
    // serial_printf("[TYPE]  math_deg_to_rad(float)       = %f\n", (double)math_deg_to_rad(f_deg));
    // serial_printf("[TYPE]  math_deg_to_rad(double)      = %f\n", math_deg_to_rad(d_deg));
    // serial_printf("[TYPE]  math_deg_to_rad(long double) = %Lf\n", math_deg_to_rad(ld_deg));

    // serial_printf("===============================================================\n\n");

    while (1) {
        thread_yield();
    }
}

void kernel_init(const uint64_t magic, const uint64_t info_ptr) {
    serial_init();
    log_debug("start");
    gdt_init();
    log_debug("gdt");
    idt_init();
    log_debug("idt");
    irq_init();
    log_debug("irq");
    fpu_init();
    log_debug("fpu");

    arch_sti(); // idt enable (turn on interrupts)

    serial_printf("MULTIBOOT2: magic=%x\n", magic);
    serial_printf("MULTIBOOT2: info_ptr=%x\n", info_ptr);

    multiboot_parse(magic, info_ptr);

    pmm_init();
    vmm_init();
    heap_init();

    phys_addr_t rsdp_phys_addr = get_rsdp_phys_addr();
    if (rsdp_phys_addr == 0) {
        serial_printf("ACPI: no RSDP address found in the MULTIBOOT2 info structure\n");
        panic();
    }

    acpi_init(rsdp_phys_addr);

    apic_init();
    hpet_init();
    rtc_init();

    pci_init();

    lapic_init(1000);

    scheduler_init();

    smp_init();

    serial_printf("INIT: Multi-Core Scheduling running!\n");

    thread_create_on_cpu(kernel_process, kernel_init_thread, NULL, "kernel_init", -1);

    scheduler_enable();
    scheduler_thread_exit();

    // Unerreichbarer Code (Fallback)

    while (1) {
        arch_hlt();
    }
}
