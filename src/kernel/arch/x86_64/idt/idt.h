/**
 * @file idt.h
 * @brief IDT definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define IDT_ENTRIES 256
#define IDT_GATE_INTERRUPT 0x8E

/** @brief Represents a single descriptor entry in the IDT (16 bytes in x86_64 long mode). */
struct idt_entry {
    uint16_t base_low;  /**< Lower 16 bits of the ISR address */
    uint16_t selector;  /**< Code segment selector in the GDT */
    uint8_t ist;        /**< Interrupt Stack Table (IST) index (bits 0-2) */
    uint8_t flags;      /**< Attributes and access rights (type, DPL, present) */
    uint16_t base_mid;  /**< Middle 16 bits of the ISR address (bits 16-31) */
    uint32_t base_high; /**< Upper 32 bits of the ISR address (bits 32-63) */
    uint32_t reserved;  /**< Reserved (must be zero) */
} __attribute__((packed));

/** @brief Represents the structure of the IDTR register for loading via the `lidt` instruction. */
struct idt_ptr {
    uint16_t limit; /**< Size of the IDT in bytes minus 1 */
    uint64_t base;  /**< Linear virtual address of the first IDT entry */
} __attribute__((packed));
