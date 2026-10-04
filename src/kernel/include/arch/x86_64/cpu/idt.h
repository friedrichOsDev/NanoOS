/**
 * @file idt.h
 * @brief Schnittstelle zur Initialisierung des Interrupt Descriptor Table (IDT).
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

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

/**
 * @brief Lädt die IDT-Adresse und das Limit mittels `lidt`-Assemblerbefehl in das IDTR-Register.
 * @param idt_ptr Virtuelle Adresse der `idt_ptr`-Struktur.
 */
extern void idt_load(uint64_t idt_ptr);

/** @brief Assembly-Stub zur Behandlung von Spurious Interrupts. */
extern void spurious_handler_stub();

/** @brief Assembly-Stub zur Behandlung von Reschedule-IPIs. */
extern void ipi_reschedule_stub();

/** @brief Assembly-Stub zur Behandlung von Stop-IPIs. */
extern void ipi_stop_stub();

/** @brief Assembly-Stub zur Behandlung von TLB-Shootdown-IPIs. */
extern void ipi_tlb_shootdown_stub();

/** @brief Assembly-Stub zur Behandlung des LAPIC Timer-Interrupts. */
extern void lapic_timer_stub();

/**
 * @brief Initialisiert die IDT, registriert Standard-Exceptions, IPIs sowie Timer-Handler und lädt die IDT.
 */
void idt_init();

/**
 * @brief Konfiguriert einen einzelnen Gate-Eintrag innerhalb der IDT.
 * @param num Vektor-Index des IDT-Eintrags (0 bis 255).
 * @param base Virtuelle Zieladresse der ISR-Stub-Funktion.
 * @param selector Code-Segment-Selektor (z. B. Kernel Code Segment).
 * @param ist Interrupt Stack Table (IST) Offsets (0 bis 7).
 * @param flags Gate-Attribute und Zugriffsrechte (z. B. `IDT_GATE_INTERRUPT`).
 */
void idt_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t ist, uint8_t flags);