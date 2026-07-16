/**
 * @file hpet.h
 * @brief High Precision Event Timer Setup (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint64_t configuration_and_capability;
    uint64_t comparator_value;
    uint64_t fsb_interrupt_route;
    uint64_t reserved;
} __attribute__((packed)) hpet_timer_t;

typedef struct {
    uint64_t general_capabilities;
    uint64_t reserved0;
    uint64_t general_configuration;
    uint64_t reserved1;
    uint64_t general_interrupt_status;
    uint8_t  reserved2[200];
    uint64_t main_counter_value;
    uint64_t reserved3;

    hpet_timer_t timers[];
} __attribute__((packed)) hpet_registers_t;

extern volatile hpet_registers_t* hpet_regs;
extern uint64_t hpet_ticks_per_us;
extern uint64_t hpet_ticks_per_ms;

void hpet_init();
void hpet_udelay(uint64_t microseconds);
void hpet_mdelay(uint64_t milliseconds);