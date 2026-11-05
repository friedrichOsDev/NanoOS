/**
 * @file hpet.h
 * @brief HPET registers and definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Represents the registers of an individual HPET timer / comparator. */
typedef struct {
    uint64_t configuration_and_capability; /**< Configuration and capabilities of the timer */
    uint64_t comparator_value;             /**< Comparator value for interrupt triggering */
    uint64_t fsb_interrupt_route;          /**< FSB/MSI interrupt routing configuration */
    uint64_t reserved;                     /**< Reserved field */
} __attribute__((packed)) hpet_timer_t;

/** @brief Structure mapping the MMIO register block of the HPET. */
typedef struct {
    uint64_t general_capabilities;     /**< General capabilities and period length (in femtoseconds) */
    uint64_t reserved0;                /**< Reserved */
    uint64_t general_configuration;    /**< General HPET configuration (Enable/Disable, Legacy Replacement) */
    uint64_t reserved1;                /**< Reserved */
    uint64_t general_interrupt_status; /**< Interrupt status of all timers */
    uint8_t reserved2[200];            /**< Reserved offset range up to the main counter */
    uint64_t main_counter_value;       /**< Current value of the HPET main counter */
    uint64_t reserved3;                /**< Reserved */
    hpet_timer_t timers[];             /**< Flexible array of available HPET timers */
} __attribute__((packed)) hpet_registers_t;

extern volatile hpet_registers_t *hpet_regs;
extern uint64_t hpet_ticks_per_us;
extern uint64_t hpet_ticks_per_ms;
extern uint64_t hpet_frequency_hz;
