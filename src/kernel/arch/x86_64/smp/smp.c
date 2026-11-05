/**
 * @file smp.c
 * @brief Symmetric Multiprocessing (SMP) implementation and AP core initialization.
 * @author friedrichOsDev
 */

#include "../acpi/acpi.h"
#include "../apic/apic.h"
#include "../gdt/gdt_sym.h"
#include "../idt/int_sym.h"
#include "../mm/mem_def.h"
#include "smp_sym.h"
#include <arch/arch.h>
#include <kernel/panic.h>
#include <lib/print.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <sched/process.h>
#include <sched/sched.h>

extern struct gdt_ptr gdtp;
extern struct idt_ptr idtp;

cpu_local_t cpus[MAX_CPUS];
size_t smp_cpu_count = 1;

static volatile bool ap_boot_flag = false;

/** @brief Enables and configures the Local APIC (LAPIC) for the current core. */
static void smp_enable_lapic() {
    uint32_t low, high;
    ASM_CODE("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));
    low |= (1 << 11);
    ASM_CODE("wrmsr" : : "a"(low), "d"(high), "c"(0x1B));

    lapic_write(LAPIC_REG_DFR, 0xFFFFFFFF);
    lapic_write(LAPIC_REG_LDR, (lapic_read(LAPIC_REG_LDR) & 0x00FFFFFF) | (1 << 24));
    lapic_write(LAPIC_REG_SIVR, lapic_read(LAPIC_REG_SIVR) | 0x100 | 0xFF);
    lapic_write(LAPIC_REG_TPR, 0);
}

/**
 * @brief Retrieves the logical CPU ID corresponding to the executing core's Local APIC ID.
 *
 * @return The logical CPU ID.
 */
uint32_t smp_get_cpu_id() {
    uint32_t lapic_id = lapic_get_id();
    for (size_t i = 0; i < smp_cpu_count; i++) {
        if (cpus[i].lapic_id == lapic_id) {
            return cpus[i].cpu_id;
        }
    }
    log_error("failed to find CPU ID for lapic ID %d", lapic_id);
    return 0;
}

/**
 * @brief Retrieves the total number of detected active CPU cores.
 *
 * @return Total CPU count.
 */
size_t smp_get_cpu_count() {
    return smp_cpu_count;
}

/**
 * @brief Retrieves the per-CPU local data structure for the currently executing core.
 *
 * @return Pointer to the current core's cpu_local_t structure.
 */
cpu_local_t *smp_get_current_cpu() {
    uint32_t lapic_id = lapic_get_id();
    for (size_t i = 0; i < smp_cpu_count; i++) {
        if (cpus[i].lapic_id == lapic_id) {
            return &cpus[i];
        }
    }
    log_error("failed to find CPU structure for lapic ID %d", lapic_id);
    return nullptr;
}

/**
 * @brief Retrieves the per-CPU local data structure for a specified logical CPU ID.
 *
 * @param[in] cpu_id Logical CPU ID.
 * @return Pointer to the requested core's cpu_local_t structure, or nullptr if out of bounds.
 */
cpu_local_t *smp_get_cpu_by_id(uint32_t cpu_id) {
    if (cpu_id < smp_cpu_count) {
        return &cpus[cpu_id];
    }
    log_error("failed to find CPU structure for CPU ID %d", cpu_id);
    return nullptr;
}

/** @brief C-level entry point executed by Application Processors (APs) upon successful startup. */
static void smp_ap_main() {
    gdt_flush((uint64_t)&gdtp);
    idt_load((uint64_t)&idtp);

    fpu_init();

    cpu_local_t *local_cpu = smp_get_current_cpu();
    if (!local_cpu) {
        log_panic("failed to get current CPU structure for an AP, cannot boot kernel");
    }

    gdt_init_core(local_cpu->cpu_id, local_cpu->kernel_stack);

    // Initialize Local APIC and timer for this AP
    smp_enable_lapic();
    lapic_timer_start_ap();

    // Create an independent idle task for this CPU core
    char idle_name[16];
    snprintf(idle_name, sizeof(idle_name), "idle_%d", local_cpu->cpu_id);
    thread_t *idle_th = NULL;
    kerr_t idle_err = thread_create_on_cpu(proc_get_kernel_proc(), idle_task, NULL, idle_name, (int)local_cpu->cpu_id, &idle_th);
    if (k_failed(idle_err)) {
        log_panic("failed to create idle thread for AP %zu", local_cpu->cpu_id);
    }
    local_cpu->idle_thread = idle_th;
    pop_next_ready_thread_for_cpu((int)local_cpu->cpu_id);

    // Allocate thread structure for the currently running AP context
    virt_addr_t mem;
    kerr_t err = kzalloc(sizeof(thread_t), &mem);
    if (k_failed(err)) {
        log_panic("failed to allocate memory for thread structure for AP main thread, cannot boot kernel");
    }
    thread_t *ap_main_thread = (thread_t *)mem;
    ap_main_thread->tid = 1000 + local_cpu->cpu_id; // Dummy TID
    snprintf(ap_main_thread->name, sizeof(ap_main_thread->name), "idle_ap_%d", local_cpu->cpu_id);
    ap_main_thread->state = THREAD_RUNNING;
    ap_main_thread->process = proc_get_kernel_proc();
    ap_main_thread->time_slice = DEFAULT_TIME_SLICE;
    ap_main_thread->cpu_affinity = (int)local_cpu->cpu_id;
    fpu_state_init(ap_main_thread->fpu_state);

    ap_main_thread->kernel_stack_top = local_cpu->kernel_stack;

    local_cpu->current_thread = ap_main_thread;

    local_cpu->online = true;
    ap_boot_flag = true;

    log_debug("CPU core %d (LAPIC ID %d) booted successfully", local_cpu->cpu_id, local_cpu->lapic_id);

    // Enable interrupts
    ASM_CODE("sti");

    // Start scheduler loop for this core
    while (1) {
        sched_schedule();
        ASM_CODE("hlt");
    }
}

/**
 * @brief Initializes Symmetric Multiprocessing (SMP), parses the MADT, and boots all available Application Processors (APs).
 *
 * @return Returns E_OK on success, or an error code (E_NOINIT, E_BADCOUNT) on failure/single-core fallback.
 */
kerr_t smp_init() {
    if (!madt) {
        log_warn("no MADT table found, staying in single-core mode");
        return E_NOINIT;
    }

    uint32_t bsp_lapic_id = lapic_get_id();

    // Register Bootstrap Processor (BSP / Core 0)
    cpus[0].cpu_id = 0;
    cpus[0].lapic_id = bsp_lapic_id;
    cpus[0].kernel_stack = (uint64_t)kernel_stack_top;
    k_assert((uint64_t)kernel_stack_top % 16 == 0);
    cpus[0].online = true;

    // Parse MADT entries and register all additional AP cores
    uint8_t *ptr = madt->entries;
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    while (ptr < end) {
        madt_entry_header_t *entry = (madt_entry_header_t *)ptr;
        if (entry->type == MADT_LAPIC_TYPE) {
            madt_lapic_entry_t *lapic = (madt_lapic_entry_t *)entry;
            // Bit 0 in Flags = Processor Enabled
            if ((lapic->flags & 1) && lapic->apic_id != bsp_lapic_id) {
                if (smp_cpu_count < MAX_CPUS) {
                    cpus[smp_cpu_count].cpu_id = (uint32_t)smp_cpu_count;
                    cpus[smp_cpu_count].lapic_id = lapic->apic_id;
                    cpus[smp_cpu_count].kernel_stack = 0;
                    cpus[smp_cpu_count].online = false;
                    smp_cpu_count++;
                }
            }
        }
        ptr += entry->length;
    }

    log_debug("detected %d CPU core(s)", smp_cpu_count);
    if (smp_cpu_count == 1) {
        log_warn("only one CPU core detected, staying in single-core mode");
        return E_BADCOUNT;
    }

    // Copy real-mode trampoline code to 0x8000
    size_t trampoline_size = (size_t)(smp_trampoline_end - smp_trampoline_start);
    memcpy((void *)P2V(0x8000), smp_trampoline_start, trampoline_size);

    // Boot all detected AP cores sequentially
    for (size_t i = 1; i < smp_cpu_count; i++) {
        uint32_t target_apic_id = cpus[i].lapic_id;

        if (target_apic_id == bsp_lapic_id) {
            continue;
        }

        // Allocate dedicated kernel stack for the core
        virt_addr_t mem;
        K_TRY(kzalloc(STACK_SIZE, &mem));
        void *ap_stack = (void *)mem;

        uint64_t ap_stack_top = (uint64_t)ap_stack + STACK_SIZE;
        k_assert(ap_stack_top % 16 == 0);
        cpus[i].kernel_stack = ap_stack_top;

        // Populate trampoline parameters in real-mode memory region (0x8000)
        uint64_t *trampoline_vars = (uint64_t *)P2V(0x8000 + ((uint64_t)&smp_trampoline_pml4 - (uint64_t)smp_trampoline_start));

        trampoline_vars[0] = vmm_kernel_space()->pml4_phys; // Page table
        trampoline_vars[1] = ap_stack_top;                  // Stack pointer
        trampoline_vars[2] = (uint64_t)smp_ap_main;         // Target entry

        ap_boot_flag = false;

        log_debug("booting core %d (LAPIC ID %d)", i, target_apic_id);

        // Send standard INIT-SIPI-SIPI sequence
        lapic_send_init(target_apic_id);
        hpet_mdelay(10); // Wait 10 ms

        // Send startup IPI with vector 0x08 (Address: 0x08 * 4096 = 0x8000)
        lapic_send_sipi(target_apic_id, 0x08);
        hpet_udelay(200); // Wait 200 µs

        if (!ap_boot_flag) {
            lapic_send_sipi(target_apic_id, 0x08);
            hpet_mdelay(10);
        }

        // Wait up to 50 ms for core acknowledgement
        uint64_t timeout = 50;
        while (!ap_boot_flag && timeout > 0) {
            hpet_mdelay(1);
            timeout--;
        }

        if (cpus[i].online) {
            log_debug("core %d started successfully", i);
        } else {
            log_error("core %d failed to start", i);
            K_TRY(kfree((virt_addr_t)ap_stack));
        }
    }

    log_debug("all available cores started");
    return E_OK;
}
