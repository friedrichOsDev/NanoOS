/**
 * @file panic.h
 * @brief Kernel panic functions.
 * @author friedrichOsDev
 */

#pragma once

#include <arch/arch.h>

/** @brief Triggers a kernel panic, halts all CPU cores, and prints a failure message. */
static inline void panic() {
    arch_cli(); // Disable interrupts
    if (is_apic_initialized()) {
        arch_ipi_stop_others();
    }
    serial_printf("KERNEL PANIC :(\n");
    while (1) {
        arch_hlt();
    }
}
