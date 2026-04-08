/**
 * @file idt.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define IDT_ENTRIES 256 /**< Total number of entries in the IDT. */

/** @brief Structure representing an IDT entry (gate). */
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

/** @brief Structure representing the IDT pointer for the LIDT instruction. */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
