/**
 * @file apic.h
 * @brief Advanced Programmable Interrupt Controller Code (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define IOAPIC_DEFAULT_PHYS 0xFEC00000

// LAPIC register offsets
#define LAPIC_REG_ID 0x0020
#define LAPIC_REG_VERSION 0x0030
#define LAPIC_REG_TPR 0x0080
#define LAPIC_REG_EOI 0x00B0
#define LAPIC_REG_LDR 0x00D0
#define LAPIC_REG_DFR 0x00E0
#define LAPIC_REG_SIVR 0x00F0
#define LAPIC_REG_ICR_LOW 0x0300
#define LAPIC_REG_ICR_HIGH 0x0310

// LAPIC Timer registers
#define LAPIC_REG_TIMER_LVT     0x0320  // LVT Timer Register
#define LAPIC_REG_TIMER_INITCNT 0x0380  // Initial Count Register
#define LAPIC_REG_TIMER_CURRCNT 0x0390  // Current Count Register
#define LAPIC_REG_TIMER_DIV     0x03E0  // Divide Configuration Register

// LAPIC Timer modes (bits 17:18 of LVT Timer)
#define LAPIC_TIMER_PERIODIC    (1 << 17)
#define LAPIC_TIMER_MASKED      (1 << 16)

// LAPIC Timer divider values for DIV register
#define LAPIC_TIMER_DIV_1   0x0B
#define LAPIC_TIMER_DIV_2   0x00
#define LAPIC_TIMER_DIV_4   0x01
#define LAPIC_TIMER_DIV_8   0x02
#define LAPIC_TIMER_DIV_16  0x03
#define LAPIC_TIMER_DIV_32  0x08
#define LAPIC_TIMER_DIV_64  0x09
#define LAPIC_TIMER_DIV_128 0x0A

// Timer interrupt vector
#define LAPIC_TIMER_VECTOR 0xFE

// I/O-APIC register offsets
#define IOAPIC_REG_INDEX 0x00
#define IOAPIC_REG_DATA 0x10
#define IOAPIC_REG_ID 0x00
#define IOAPIC_REG_VER 0x01
#define IOAPIC_REG_ARB 0x02
#define IOAPIC_REG_RED_TABLE(idx) (0x10 + (idx) * 2)

// IPI vectors
#define IPI_RESCHEDULE_VECTOR 0xFD
#define IPI_STOP_VECTOR 0xFC

void apic_init(void);
void lapic_write(uint32_t reg, uint32_t val);
uint32_t lapic_read(uint32_t reg);
void lapic_eoi(void);
void ioapic_route_irq(uint8_t irq, uint8_t vector, uint8_t cpu_id);
void lapic_send_init(uint32_t lapic_id);
void lapic_send_sipi(uint32_t lapic_id, uint8_t vector);
uint32_t lapic_get_id(void);
void lapic_send_broadcast_reschedule_ipi();
void lapic_send_broadcast_stop_ipi();
void lapic_timer_calibrate_and_start(uint32_t target_hz);
void lapic_timer_start_ap(void);

extern uint32_t lapic_timer_calibrated_initcnt;
extern uint32_t lapic_timer_target_hz;