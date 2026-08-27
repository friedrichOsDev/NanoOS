/**
 * @file hpet.c
 * @brief High Precision Event Timer Setup (Header)
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/vmm.h>

volatile hpet_registers_t *hpet_regs = NULL;
uint64_t hpet_ticks_per_us = 0;
uint64_t hpet_ticks_per_ms = 0;

/**
 * Initializes the HPET
 */
void hpet_init() {
    if (!hpet) {
        serial_printf(COM1, "HPET: error HPET table is not initialized\n");
        return;
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
        serial_printf(COM1, "HPET: error failed to map HPET registers\n");
        return;
    }

    // 1. Calculate ticks per us and ms
    uint32_t tick_period = hpet_regs->general_capabilities >> 32;
    if (tick_period == 0) {
        serial_printf(COM1, "HPET: tick period is 0!\n");
        return;
    }

    hpet_ticks_per_us = 1000000000ULL / tick_period;
    hpet_ticks_per_ms = 1000000000000ULL / tick_period;
    serial_printf(COM1,
                  "HPET: tick period: %u fs, ticks/us: %llu, ticks/ms: %llu\n",
                  tick_period, hpet_ticks_per_us, hpet_ticks_per_ms);

    // 2. Stop the main counter before configuring
    hpet_regs->general_configuration &= ~1ULL;

    // 3. Clear the main counter value
    hpet_regs->main_counter_value = 0;

    // 4. Configure Timer 0 for periodic mode generating interrupts every 1ms
    if (hpet_regs->timers[0].configuration_and_capability & (1 << 4)) {
        serial_printf(COM1, "HPET: Timer 0 supports periodic mode\n");

        uint64_t timer_conf = hpet_regs->timers[0].configuration_and_capability;
        timer_conf |= (1 << 2); // Enable Interrupts
        timer_conf |= (1 << 3); // Periodic Mode
        timer_conf |= (1 << 6); // Value Set (allows setting period)

        hpet_regs->timers[0].configuration_and_capability = timer_conf;
        hpet_regs->timers[0].comparator_value = hpet_ticks_per_ms;
    } else {
        serial_printf(
            COM1, "HPET: warning Timer 0 does NOT support periodic mode!\n");
    }

    // 5. Enable legacy replacement routing if supported
    if (hpet_regs->general_capabilities & (1 << 15)) {
        hpet_regs->general_configuration |= (1 << 1);
        serial_printf(COM1, "HPET: Legacy Replacement Routing enabled\n");
    }

    // 6. Start the main counter
    hpet_regs->general_configuration |= 1ULL;
    serial_printf(COM1, "HPET: main counter started\n");
}

/**
 * A precise microsecond delay
 * @param microseconds The time to be waited in μs
 */
void hpet_udelay(uint64_t microseconds) {
    if (!hpet_regs)
        return;
    uint64_t target_ticks =
        hpet_regs->main_counter_value + (microseconds * hpet_ticks_per_us);

    while (hpet_regs->main_counter_value < target_ticks) {
        __asm__ __volatile__("pause");
    }
}

/**
 * A precise millisecond delay
 * @param milliseconds The time to be waited in ms
 */
void hpet_mdelay(uint64_t milliseconds) {
    if (!hpet_regs)
        return;
    uint64_t target_ticks =
        hpet_regs->main_counter_value + (milliseconds * hpet_ticks_per_ms);

    while (hpet_regs->main_counter_value < target_ticks) {
        __asm__ __volatile__("pause");
    }
}
