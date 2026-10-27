/**
 * @file idt.h
 * @brief Schnittstelle zur Initialisierung des Interrupt Descriptor Table (IDT).
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Gesamtanzahl der IDT-Einträge (0-255). */
#define IDT_ENTRIES 256

/** @brief Flag-Maske für ein 64-Bit Interrupt-Gate im Kernel-Mode (Present, Ring 0, Interrupt Gate). */
#define IDT_GATE_INTERRUPT 0x8E

/**
 * @brief Repräsentiert einen einzelnen Deskriptor-Eintrag in der IDT (16 Bytes im x86_64 Long Mode).
 */
struct idt_entry {
    uint16_t base_low;  /**< Untere 16 Bits der ISR-Adresse */
    uint16_t selector;  /**< Code-Segment-Selektor in der GDT */
    uint8_t ist;        /**< Interrupt Stack Table (IST) Index (Bits 0-2) */
    uint8_t flags;      /**< Attribute und Zugriffsrechte (Type, DPL, Present) */
    uint16_t base_mid;  /**< Mittlere 16 Bits der ISR-Adresse (Bits 16-31) */
    uint32_t base_high; /**< Obere 32 Bits der ISR-Adresse (Bits 32-63) */
    uint32_t reserved;  /**< Reserviert (muss 0 sein) */
} __attribute__((packed));

/**
 * @brief Repräsentiert die Struktur des IDTR-Registers zur Übergabe an den `lidt`-Befehl.
 */
struct idt_ptr {
    uint16_t limit; /**< Größe der IDT in Bytes minus 1 */
    uint64_t base;  /**< Lineare Virtuelle Adresse des ersten IDT-Eintrags */
} __attribute__((packed));
