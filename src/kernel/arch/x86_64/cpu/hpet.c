/**
 * @file hpet.c
 * @brief High Precision Event Timer Setup (Header)
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/panic.h>

volatile hpet_registers_t *hpet_regs = NULL;
uint64_t hpet_ticks_per_us = 0;
uint64_t hpet_ticks_per_ms = 0;
uint64_t hpet_frequency_hz = 0;

void hpet_init() {
    if (!hpet) {
        panic("FATAL: System requires HPET, but none was found in ACPI tables.", 0);
    }

    uint64_t hpet_phys_addr = hpet->base_address.address;
    if (hpet->base_address.address_space_id != 0) {
        serial_printf(COM1, "HPET: warning HPET isn't in MMIO space\n");
    }

    hpet_regs = (volatile hpet_registers_t *)vmm_map_mmio(
        (page_table_t *)kernel_pml4, hpet_phys_addr, 1);
    serial_printf(COM1, "HPET: MMIO mapped to virtual address: %llx\n",
                  (virt_addr_t)hpet_regs);

    if (!hpet_regs) {
        panic("FATAL: Error failed to map HPET registers.", 0);
    }

    // 1. Calculate frequency from tick period (in femtoseconds)
    uint32_t tick_period_fs = hpet_regs->general_capabilities >> 32;
    if (tick_period_fs == 0) {
        panic("FATAL: tick period is 0!", 0);
    }

    hpet_ticks_per_us = 1000000000ULL / tick_period_fs;
    hpet_ticks_per_ms = 1000000000000ULL / tick_period_fs;
    hpet_frequency_hz = 1000000000000000ULL / tick_period_fs;
    serial_printf(COM1, "HPET: tick period: %u fs, frequency: %llu Hz, ticks/ms: %llu\n", tick_period_fs, hpet_frequency_hz, hpet_ticks_per_ms);

    // 2. Stop counter, reset to 0
    hpet_regs->general_configuration &= ~3ULL; // Disable counter + legacy routing
    hpet_regs->main_counter_value = 0;

    // 3. Disable all timer interrupts (we only want free-running counter)
    uint32_t num_timers = ((hpet_regs->general_capabilities >> 8) & 0x1F) + 1;
    for (uint32_t i = 0; i < num_timers; i++) {
        hpet_regs->timers[i].configuration_and_capability &= ~(1ULL << 2); // Disable IRQ
    }

    // 4. Start the free-running counter (bit 0 only, no legacy routing)
    hpet_regs->general_configuration |= 1ULL;

    serial_printf(COM1, "HPET: free-running counter started (%d timers disabled)\n", num_timers);
}

/**
 * Returns the value of the main counter
 */
uint64_t hpet_read_counter(void) {
    if (!hpet_regs) panic("FATAL: Error failed to map HPET registers.", 0);
    return hpet_regs->main_counter_value;
}

void hpet_udelay(uint64_t microseconds) {
    if (!hpet_regs) panic("FATAL: Error failed to map HPET registers.", 0);
    uint64_t target = hpet_regs->main_counter_value + (microseconds * hpet_ticks_per_us);
    while (hpet_regs->main_counter_value < target) {
        __asm__ __volatile__("pause");
    }
}

void hpet_mdelay(uint64_t milliseconds) {
    if (!hpet_regs) panic("FATAL: Error failed to map HPET registers.", 0);
    uint64_t target = hpet_regs->main_counter_value + (milliseconds * hpet_ticks_per_ms);
    while (hpet_regs->main_counter_value < target) {
        __asm__ __volatile__("pause");
    }
}

uint64_t hpet_uptime_ms(void) {
    if (!hpet_regs || hpet_ticks_per_ms == 0) return 0;
    return hpet_regs->main_counter_value / hpet_ticks_per_ms;
}

uint64_t hpet_uptime_us(void) {
    if (!hpet_regs || hpet_ticks_per_us == 0) return 0;
    return hpet_regs->main_counter_value / hpet_ticks_per_us;
}