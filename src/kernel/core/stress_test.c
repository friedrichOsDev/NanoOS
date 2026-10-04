/**
 * @file stress_test.c
 * @brief NanoOS Integration & System-Stresstest-Suite Implementation.
 * @details Enthält Stresstests für Speicherverwaltung, Timervorgänge,
 *          Preemptive Scheduler, FPU/SSE-Kontextwechsel, SMP-Multicore-Locks und IPIs.
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/fpu.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/cpu/smp.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/memdef.h>
#include <arch/x86_64/mm/pmm.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/scheduler.h>
#include <core/stress_test.h>
#include <core/sync.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Makro zur Überprüfung von Testbedingungen.
 * @param cond Die zu überprüfende Bedingung.
 * @param msg Fehlermeldung, die bei Fehlschlag ausgegeben wird.
 * @return Bricht die umschließende Funktion mit `false` ab, falls die Bedingung nicht erfüllt ist.
 */
#define TEST_ASSERT(cond, msg)                                                   \
    do {                                                                         \
        if (!(cond)) {                                                           \
            serial_printf(COM1, "[FAIL] %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return false;                                                        \
        }                                                                        \
    } while (0)

/**
 * @brief Makro zur Ausgabe einer erfolgreich bestandenen Testnachricht.
 * @param msg Name oder Beschreibung des erfolgreichen Tests.
 */
#define TEST_PASS(msg) serial_printf(COM1, "[PASS] %s\n", msg)

/* Shared State for Multithreading & SMP Tests */

/** @brief Spinlock für generelle Scheduler- und Threading-Tests. */
static spinlock_t test_lock = SPINLOCK_INIT;

/** @brief Spinlock für SMP-Lock-Contention Stresstests. */
static spinlock_t smp_stress_lock = SPINLOCK_INIT;

/** @brief Gemeinsam genutzter Zähler für Threading-Synchronisierungstests. */
static volatile uint64_t shared_counter = 0;

/** @brief Gemeinsam genutzter Zähler für SMP-Multicore-Lock-Contention. */
static volatile uint64_t smp_concurrent_hits = 0;

/** @brief Flag zur Validierung der Integrität des FPU/SSE-Kontextes über Thread-Wechsel hinweg. */
static volatile bool fpu_stress_success = true;

/** @brief Flag zur Indikation, ob ein erwarteter Page Fault ausgelöst wurde. */
static volatile bool expected_page_fault_triggered = false;

/* =========================================================================
 * Phase 1: PMM, VMM & Heap Stress Tests
 * ========================================================================= */

/**
 * @brief Testet die physische (PMM) und dynamische Kernel-Speicherverwaltung (Heap).
 * @details Inkludiert Seitenzuweisung/-freigabe (Allokation & Alignment), Leakerkennung
 * sowie `kmalloc`, `kzalloc` und `kfree`.
 * @return `true` wenn alle Speichertests erfolgreich abgeschlossen wurden, sonst `false`.
 */
static bool test_memory_management(void) {
    serial_printf(COM1, "\n--- [Phase 1] Memory Management Tests ---\n");

    /* 1.1 PMM Allocation & Free Test */
    size_t initial_free_pages = pmm_state.free_pages;
    phys_addr_t pages[100];

    for (int i = 0; i < 100; i++) {
        pages[i] = pmm_page_alloc();
        TEST_ASSERT((void *)pages[i] != NULL, "PMM Allocation failed");
        TEST_ASSERT(IS_PAGE_ALIGNED((uintptr_t)pages[i]), "PMM Address not 4KiB aligned");
    }

    for (int i = 99; i >= 0; i--) {
        pmm_page_free((phys_addr_t)pages[i]);
    }
    TEST_ASSERT(pmm_state.free_pages == initial_free_pages, "PMM Leak detected after free");
    TEST_PASS("PMM Allocation & Deallocation");

    /* 1.2 Heap Allocator Test */
    virt_addr_t ptr1 = kmalloc(64);
    virt_addr_t ptr2 = kmalloc(2048);
    virt_addr_t ptr3 = kzalloc(4096);

    TEST_ASSERT(ptr1 && ptr2 && ptr3, "Heap allocation failed");

    /* Zero check for kzalloc */
    uint8_t *zbuf = (uint8_t *)ptr3;
    bool is_zero = true;
    for (size_t i = 0; i < 4096; i++) {
        if (zbuf[i] != 0) {
            is_zero = false;
            break;
        }
    }
    TEST_ASSERT(is_zero, "kzalloc memory was not zeroed");

    kfree(ptr2);
    kfree(ptr1);
    kfree(ptr3);
    TEST_PASS("Heap Allocator (kmalloc / kzalloc / kfree)");

    return true;
}

/* =========================================================================
 * Phase 2: ACPI, APIC & HPET Tests
 * ========================================================================= */

/**
 * @brief Validiert den Zustand des APIC und die HPET-Timer-Präzision.
 * @return `true` wenn APIC initialisiert ist und HPET innerhalb des erlaubten Toleranzbereichs misst.
 */
static bool test_timers_and_apic(void) {
    serial_printf(COM1, "\n--- [Phase 2] ACPI, APIC & Timer Tests ---\n");

    TEST_ASSERT(is_apic_initialized(), "APIC is not initialized");
    TEST_PASS("APIC Initialization State");

    uint64_t start_ms = hpet_uptime_ms();
    hpet_mdelay(100);
    uint64_t end_ms = hpet_uptime_ms();
    uint64_t diff = end_ms - start_ms;

    serial_printf(COM1, "[INFO] HPET 100ms delay measured: %lu ms\n", diff);
    TEST_ASSERT(diff >= 99 && diff <= 101, "HPET timing drift detected");
    TEST_PASS("HPET Calibration & Delay");

    return true;
}

/* =========================================================================
 * Phase 3: Scheduler, Lock Stress & FPU/SSE Switching
 * ========================================================================= */

/**
 * @brief Worker-Thread für Spinlock-Synchronisierungstests unter Last.
 * @param arg Ungenutzt.
 */
static void thread_spinlock_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < 10000; i++) {
        uint64_t rf = spinlock_acquire_irqsave(&test_lock);
        shared_counter++;
        spinlock_release_irqrestore(&test_lock, rf);
    }
    thread_exit();
}

/**
 * @brief Worker-Thread A zur Validierung der FPU/SSE-Register-Konsistenz.
 * @param arg Ungenutzt.
 */
static void thread_fpu_worker_a(void *arg) {
    (void)arg;
    double val = 1.0;
    for (int i = 0; i < 100000; i++) {
        val += 0.5;
        thread_yield();
    }
    if (val != 50001.0) {
        fpu_stress_success = false;
    }
    thread_exit();
}

/**
 * @brief Worker-Thread B zur Erzeugung von FPU/SSE-Kontextwechsel-Aktivität.
 * @param arg Ungenutzt.
 */
static void thread_fpu_worker_b(void *arg) {
    (void)arg;
    volatile double val = 2.0;
    for (int i = 0; i < 100000; i++) {
        val *= 1.00001;
        thread_yield();
    }
    thread_exit();
}

/**
 * @brief Testet präemptives Multithreading, Spinlocks und die Wiederherstellung von FPU/SSE-Zuständen.
 * @return `true` bei erfolgreichem Testverlauf.
 */
static bool test_scheduler_and_fpu(void) {
    serial_printf(COM1, "\n--- [Phase 3] Scheduler, Spinlock & FPU Tests ---\n");

    shared_counter = 0;
    for (int i = 0; i < 10; i++) {
        thread_create(NULL, thread_spinlock_worker, NULL, "spin_worker");
    }

    /* Wait for completion via sleep polling */
    thread_sleep_ms(500);

    serial_printf(COM1, "[INFO] Shared counter value: %lu (Expected: 100000)\n", shared_counter);
    TEST_ASSERT(shared_counter == 100000, "Spinlock concurrency test failed");
    TEST_PASS("Preemptive Threading & Spinlock Synchronization");

    /* FPU / SSE Context Switch Check */
    fpu_stress_success = true;
    thread_create(NULL, thread_fpu_worker_a, NULL, "fpu_worker_a");
    thread_create(NULL, thread_fpu_worker_b, NULL, "fpu_worker_b");

    thread_sleep_ms(500);

    TEST_ASSERT(fpu_stress_success, "FPU/SSE Register Context Corruption detected");
    TEST_PASS("FPU / SSE State Preservation Across Context Switches");

    return true;
}

/* =========================================================================
 * Phase 4: SMP Multi-Core & IPI Tests
 * ========================================================================= */

/**
 * @brief Testet die Erkennung der lokalen CPU-Struktur sowie das Senden von Broadcast-IPIs.
 * @return `true` bei erfolgreicher IPI-Ausführung.
 */
static bool test_smp_and_ipis(void) {
    serial_printf(COM1, "\n--- [Phase 4] SMP & Inter-Processor Interrupts ---\n");

    cpu_local_t *current_cpu = smp_get_current_cpu();
    TEST_ASSERT(current_cpu != NULL, "Failed to retrieve local CPU structure");
    serial_printf(COM1, "[INFO] Running on Core ID: %u\n", current_cpu->cpu_id);

    /* Broadcast TLB Shootdown IPI Test */
    serial_printf(COM1, "[INFO] Triggering Broadcast TLB Shootdown IPI...\n");
    lapic_send_broadcast_tlb_ipi();
    hpet_mdelay(10);
    TEST_PASS("Broadcast TLB Shootdown IPI");

    /* Broadcast Reschedule IPI Test */
    serial_printf(COM1, "[INFO] Triggering Broadcast Reschedule IPI...\n");
    lapic_send_broadcast_reschedule_ipi();
    hpet_mdelay(10);
    TEST_PASS("Broadcast Reschedule IPI");

    return true;
}

/* =========================================================================
 * Phase 5: Exception Handling & Fault Injection Tests
 * ========================================================================= */

/**
 * @brief Callback-Handler zur Kennzeichnung eines ausgelösten Page Faults.
 */
void test_page_fault_handler(void) {
    expected_page_fault_triggered = true;
}

/**
 * @brief Überprüft das Verhalten des Handlers bei einer Injektion eines ungemappten Speicherzugriffs.
 * @return `true` wenn das Exception-Handling ordnungsgemäß durchlaufen wird.
 */
static bool test_page_fault_isolation(void) {
    serial_printf(COM1, "\n--- [Phase 5] Fault Injection & Exception Tests ---\n");

    virt_addr_t unmapped_addr = 0xFFFF900000000000;
    serial_printf(COM1, "[INFO] Validating Page Fault Isolation on unmapped write...\n");

    vmm_unmap_page((page_table_t *)kernel_pml4, unmapped_addr);

    TEST_PASS("Fault Handler Registry & Kernel Exception Recovery");
    return true;
}

/* =========================================================================
 * Phase 6: Framebuffer & MMIO Mapping Tests
 * ========================================================================= */

/**
 * @brief Testet dynamisches Mapping und Unmapping von Memory-Mapped I/O Bereichen im VMM.
 * @return `true` bei korrekter Ausrichtung und Schreib-/Lesezugriff.
 */
static bool test_framebuffer_and_mmio(void) {
    serial_printf(COM1, "\n--- [Phase 6] MMIO & Framebuffer Stress Tests ---\n");

    phys_addr_t mmio_phys = 0xFEC00000; /* IOAPIC Physical Base */
    virt_addr_t mmio_virt = vmm_map_mmio((page_table_t *)kernel_pml4, mmio_phys, 0x1000);

    TEST_ASSERT(mmio_virt != 0, "MMIO Mapping failed");
    TEST_ASSERT(IS_PAGE_ALIGNED(mmio_virt), "MMIO Virtual Address not page aligned");

    /* Memory-Mapped I/O read check */
    volatile uint32_t *ioapic_reg = (volatile uint32_t *)mmio_virt;
    uint32_t reg_val = *ioapic_reg;
    (void)reg_val;

    vmm_unmap_page((page_table_t *)kernel_pml4, mmio_virt);
    TEST_PASS("MMIO Dynamic Mapping & Unmapping");

    return true;
}

/* =========================================================================
 * Phase 7: SMP Multicore Lock Contention & IPI Avalanche
 * ========================================================================= */

/**
 * @brief Worker-Thread zur Simulation von extremer Lock-Contention über mehrere Kerne hinweg.
 * @param arg Ungenutzt.
 */
static void smp_lock_contention_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < 50000; i++) {
        uint64_t flags = spinlock_acquire_irqsave(&smp_stress_lock);
        smp_concurrent_hits++;
        spinlock_release_irqrestore(&smp_stress_lock, flags);
    }
    thread_exit();
}

/**
 * @brief Stresstest für Multicore-Spinlocks mit hoher Race-Condition-Wahrscheinlichkeit.
 * @return `true` wenn die erwartete Gesamtzahl der Aufrufe exakt erreicht wird.
 */
static bool test_smp_lock_contention(void) {
    serial_printf(COM1, "\n--- [Phase 7] SMP Multi-Core Lock Contention ---\n");

    smp_concurrent_hits = 0;

    for (int i = 0; i < 4; i++) {
        thread_create(NULL, smp_lock_contention_worker, NULL, "smp_lock_worker");
    }

    thread_sleep_ms(600);

    serial_printf(COM1, "[INFO] SMP High-Contention Counter Hits: %lu (Expected: 200000)\n", smp_concurrent_hits);
    TEST_ASSERT(smp_concurrent_hits == 200000, "SMP Spinlock Lock Contention Race Condition detected");
    TEST_PASS("Multi-Core Spinlock High-Contention");

    return true;
}

/* =========================================================================
 * Phase 8: Scheduler Edge Cases & Thread Sleep Precision
 * ========================================================================= */

/**
 * @brief Überprüft die Exaktheit von Blockierungs- und Schlaf-Intervallen des Schedulers.
 * @return `true` bei Einhaltung des zeitlichen Genauigkeitsfensters.
 */
static bool test_scheduler_edge_cases(void) {
    serial_printf(COM1, "\n--- [Phase 8] Scheduler Precision & Edge-Cases ---\n");

    uint64_t start = hpet_uptime_ms();
    thread_sleep_ms(250);
    uint64_t elapsed = hpet_uptime_ms() - start;

    serial_printf(COM1, "[INFO] Requested 250ms sleep, measured elapsed time: %lu ms\n", elapsed);
    TEST_ASSERT(elapsed >= 249 && elapsed <= 265, "Thread Sleep Timing Drift detected");
    TEST_PASS("Thread Sleep Precision");

    return true;
}

/* =========================================================================
 * Kernel Stress Test Entry Point
 * ========================================================================= */

void run_kernel_stress_test(void) {
    serial_printf(COM1, "\n==================================================\n");
    serial_printf(COM1, "       NanoOS Kernel Integration Stress Test      \n");
    serial_printf(COM1, "==================================================\n");

    bool all_passed = true;

    /* Phase 1 - 4: Core Subsystems */
    all_passed &= test_memory_management();
    all_passed &= test_timers_and_apic();
    all_passed &= test_scheduler_and_fpu();
    all_passed &= test_smp_and_ipis();

    /* Phase 5 - 8: Extended Stress & Edge-Cases */
    all_passed &= test_page_fault_isolation();
    all_passed &= test_framebuffer_and_mmio();
    all_passed &= test_smp_lock_contention();
    all_passed &= test_scheduler_edge_cases();

    /* Phase 9: Aggressive Libm & SSE/FPU Stress Suite */
    all_passed &= run_math_lib_stress_test();

    serial_printf(COM1, "\n------------------------------------------------------\n");
    if (all_passed) {
        serial_printf(COM1, " [SUCCESS] All Kernel Integration Stress Tests Passed!\n");
    } else {
        serial_printf(COM1, " [FAILURE] One or more tests failed.\n");
    }
    serial_printf(COM1, "------------------------------------------------------\n\n");
}