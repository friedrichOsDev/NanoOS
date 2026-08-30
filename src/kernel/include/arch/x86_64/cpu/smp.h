/**
 * @file smp.h
 * @brief Symmetric Multiprocessing (SMP) header
 * @author friedrichOsDev
 */

#pragma once

#include <core/thread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_CPUS 64

typedef struct cpu_local {
    uint32_t cpu_id;          // logical CPU number (0 = BSP, 1 = AP1, ...)
    uint32_t lapic_id;        // physical APIC ID from ACPI MADT
    volatile bool online;     // Flag: core online
    thread_t *current_thread; // current thread of the core
    thread_t *idle_thread;    // dedicated idle thread for every core
} cpu_local_t;

extern cpu_local_t cpus[MAX_CPUS];
extern size_t smp_cpu_count;

void smp_init(void);
cpu_local_t *smp_get_current_cpu(void);