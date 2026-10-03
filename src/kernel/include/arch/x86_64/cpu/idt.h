/**
 * @file idt.h
 * @brief IDT Setup
 */

#pragma once

#include <stdint.h>

#define IDT_ENTRIES 256
#define IDT_GATE_INTERRUPT 0x8E

/**
 * @brief IDT Descriptor Entry Structure
 */
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

/**
 * @brief IDTR Pointer Register Structure
 */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/**
 * @brief Loads the IDT Pointer Register
 * @param idt_ptr Pointer to the IDT Pointer Register structure
 */
extern void idt_load(uint64_t idt_ptr);

/**
 * @brief Spurious Handler Stub
 */
extern void spurious_handler_stub();

/**
 * @brief IPI Reschedule Handler Stub
 */
extern void ipi_reschedule_stub();

/**
 * @brief IPI Stop Handler Stub
 */
extern void ipi_stop_stub();

/**
 * @brief IPI TLB Shootdown Handler Stub
 */
extern void ipi_tlb_shootdown_stub();

/**
 * @brief Lapic Timer Handler Stub
 */
extern void lapic_timer_stub();

/**
 * @brief Initializes the IDT
 */
void idt_init();

/**
 * @brief Configures an individual gate in the IDT
 * @param num Entry vector index (0-255)
 * @param base Address of ISR entry stub
 * @param selector Code segment selector
 * @param ist Interrupt Stack Table offset (0-7)
 * @param flags Access and descriptor gate attributes
 */
void idt_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t ist, uint8_t flags);