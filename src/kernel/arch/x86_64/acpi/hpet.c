/**
 * @file hpet.c
 * @brief Driver for the High Precision Event Timer (HPET).
 * @author friedrichOsDev
 */

#include "hpet.h"
#include "acpi.h"
#include <arch/arch.h>
#include <kernel/panic.h>
#include <mm/memory.h>

volatile hpet_registers_t *hpet_regs = NULL;
uint64_t hpet_ticks_per_us = 0;
uint64_t hpet_ticks_per_ms = 0;
uint64_t hpet_frequency_hz = 0;

/** @brief Initialize the HPET. */
void hpet_init() {
    if (!hpet)
        log_panic("unable to boot, HPET is missing");

    uint64_t hpet_phys_addr = hpet->base_address.address;
    if (hpet->base_address.address_space_id != 0)
        log_warn("HPET base address is not in physical memory");

    virt_addr_t mem;
    kerr_t err = vmm_map_mmio(vmm_kernel_space(), hpet_phys_addr, 1, &mem);
    if (k_failed(err))
        log_panic("unable to map HPET registers");

    hpet_regs = (volatile hpet_registers_t *)(void *)mem;
    log_debug("HPET mapped to virtual address: %llx", (virt_addr_t)hpet_regs);

    uint32_t tick_period_fs = (uint32_t)(hpet_regs->general_capabilities >> 32);
    k_assert(tick_period_fs != 0);

    hpet_ticks_per_us = 1000000000ULL / tick_period_fs;
    hpet_ticks_per_ms = 1000000000000ULL / tick_period_fs;
    hpet_frequency_hz = 1000000000000000ULL / tick_period_fs;
    log_debug("HPET tick period: %u fs, frequency: %llu Hz, ticks/ms: %llu", tick_period_fs, hpet_frequency_hz, hpet_ticks_per_ms);

    hpet_regs->general_configuration &= ~3ULL; // disable legacy routing, stop main counter
    hpet_regs->main_counter_value = 0;

    uint32_t num_timers = (uint32_t)(((hpet_regs->general_capabilities >> 8) & 0x1F) + 1);
    for (uint32_t i = 0; i < num_timers; i++) {
        hpet_regs->timers[i].configuration_and_capability &= ~(1ULL << 2); // disable timer IRQ
    }

    hpet_regs->general_configuration |= 1ULL;

    log_debug("HPET free-running counter started (%u timers disabled)", num_timers);
}

/**
 * @brief Read the current value of the HPET free-running counter.
 *
 * @return The current value of the HPET free-running counter
 */
uint64_t hpet_read_counter() {
    if (!hpet_regs) {
        log_warn("HPET may be uninitialized");
        return 0;
    }
    return hpet_regs->main_counter_value;
}

/**
 * @brief Delay for a specified number of microseconds.
 *
 * @param microseconds The number of microseconds to delay.
 */
void hpet_udelay(uint64_t microseconds) {
    if (!hpet_regs) {
        log_warn("HPET may be uninitialized");
        return;
    }
    uint64_t target = hpet_regs->main_counter_value + (microseconds * hpet_ticks_per_us);
    while (hpet_regs->main_counter_value < target) {
        arch_pause();
    }
}

/**
 * @brief Delay for a specified number of milliseconds.
 *
 * @param milliseconds The number of milliseconds to delay.
 */
void hpet_mdelay(uint64_t milliseconds) {
    if (!hpet_regs) {
        log_warn("HPET may be uninitialized");
        return;
    }
    uint64_t target = hpet_regs->main_counter_value + (milliseconds * hpet_ticks_per_ms);
    while (hpet_regs->main_counter_value < target) {
        arch_pause();
    }
}

/**
 * @brief Get the current uptime in milliseconds.
 *
 * @return The current uptime in milliseconds.
 */
uint64_t hpet_uptime_ms() {
    if (!hpet_regs || hpet_ticks_per_ms == 0) {
        log_warn("HPET may be uninitialized");
        return 0;
    }
    return hpet_regs->main_counter_value / hpet_ticks_per_ms;
}

/**
 * @brief Get the current uptime in microseconds.
 *
 * @return The current uptime in microseconds.
 */
uint64_t hpet_uptime_us() {
    if (!hpet_regs || hpet_ticks_per_us == 0) {
        log_warn("HPET may be uninitialized");
        return 0;
    }
    return hpet_regs->main_counter_value / hpet_ticks_per_us;
}

/**
 * @brief Get the current timestamp in milliseconds.
 *
 * @return The current timestamp in milliseconds.
 */
uint64_t hpet_get_timestamp() {
    return hpet_uptime_ms();
}
