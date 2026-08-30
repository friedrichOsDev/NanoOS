/**
 * @file apic.h
 * @brief Advanced Programmable Interrupt Controller Code (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define IOAPIC_DEFAULT_PHYS 0xFEC00000

// LAPIC register offsets (16 byte each)
#define LAPIC_REG_ID 0x0020      // LAPIC ID register offset
#define LAPIC_REG_VERSION 0x0030 // LAPIC Version register offset
#define LAPIC_REG_TPR 0x0080     // LAPIC Task Priority Register offset
#define LAPIC_REG_EOI 0x00B0     // LAPIC End of Interrupt register offset
#define LAPIC_REG_LDR 0x00D0     // Logical Destination Register Offset
#define LAPIC_REG_DFR 0x00E0     // Destination Format Register Offset
#define LAPIC_REG_SIVR 0x00F0 // LAPIC Spurious Interrupt Vector Register offset
#define LAPIC_REG_ICR_LOW                                                      \
    0x0300 // LAPIC Interrupt Command Register (low) offset
#define LAPIC_REG_ICR_HIGH                                                     \
    0x0310 // LAPIC Interrupt Command Register (high) offset

// I/O-APIC register offsets
#define IOAPIC_REG_INDEX 0x00 // IOAPIC Index register offset
#define IOAPIC_REG_DATA 0x10  // IOAPIC Data register offset
#define IOAPIC_REG_ID 0x00    // IOAPIC ID register offset
#define IOAPIC_REG_VER 0x01   // IOAPIC Version register offset
#define IOAPIC_REG_ARB 0x02   // IOAPIC Arbitration register offset
#define IOAPIC_REG_RED_TABLE(idx)                                              \
    (0x10 + (idx) * 2) // Redirection Entry for IRQ idx (64-bit)

#define IPI_RESCHEDULE_VECTOR 0xFD
#define IPI_TICK_VECTOR 0xFC

void apic_init();
void lapic_write(uint32_t reg, uint32_t val);
uint32_t lapic_read(uint32_t reg);
void lapic_eoi();
void ioapic_route_irq(uint8_t irq, uint8_t vector, uint8_t cpu_id);
void lapic_send_init(uint32_t lapic_id);
void lapic_send_sipi(uint32_t lapic_id, uint8_t vector);
uint32_t lapic_get_id(void);
void lapic_send_broadcast_reschedule_ipi(void);
void lapic_send_broadcast_tick_ipi(void);