/**
 * @file apic.h
 * @brief Advanced Programmable Interrupt Controller (APIC) Treiber-Schnittstelle.
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/** @brief Standard-basierte physische Adresse des I/O-APIC */
#define IOAPIC_DEFAULT_PHYS 0xFEC00000

/* LAPIC-Register-Offsets */
#define LAPIC_REG_ID 0x0020       /**< Local APIC ID Register */
#define LAPIC_REG_VERSION 0x0030  /**< Local APIC Versions-Register */
#define LAPIC_REG_TPR 0x0080      /**< Task Priority Register (TPR) */
#define LAPIC_REG_EOI 0x00B0      /**< End of Interrupt Register (EOI) */
#define LAPIC_REG_LDR 0x00D0      /**< Logical Destination Register */
#define LAPIC_REG_DFR 0x00E0      /**< Destination Format Register */
#define LAPIC_REG_SIVR 0x00F0     /**< Spurious Interrupt Vector Register */
#define LAPIC_REG_ICR_LOW 0x0300  /**< Interrupt Command Register (Bits 0-31) */
#define LAPIC_REG_ICR_HIGH 0x0310 /**< Interrupt Command Register (Bits 32-63) */

/* LAPIC-Timer-Register */
#define LAPIC_REG_TIMER_LVT 0x0320     /**< LVT Timer Register */
#define LAPIC_REG_TIMER_INITCNT 0x0380 /**< Initial Count Register */
#define LAPIC_REG_TIMER_CURRCNT 0x0390 /**< Current Count Register */
#define LAPIC_REG_TIMER_DIV 0x03E0     /**< Divide Configuration Register */

/* LAPIC-Timer-Modi (Bits 17:18 im LVT Timer Register) */
#define LAPIC_TIMER_PERIODIC (1 << 17) /**< Periodischer Timer-Modus */
#define LAPIC_TIMER_MASKED (1 << 16)   /**< Maskiert (deaktiviert) den Timer-Interrupt */

/* Werte für das Timer-Teiler-Register (DIV) */
#define LAPIC_TIMER_DIV_1 0x0B   /**< Teiler 1 */
#define LAPIC_TIMER_DIV_2 0x00   /**< Teiler 2 */
#define LAPIC_TIMER_DIV_4 0x01   /**< Teiler 4 */
#define LAPIC_TIMER_DIV_8 0x02   /**< Teiler 8 */
#define LAPIC_TIMER_DIV_16 0x03  /**< Teiler 16 */
#define LAPIC_TIMER_DIV_32 0x08  /**< Teiler 32 */
#define LAPIC_TIMER_DIV_64 0x09  /**< Teiler 64 */
#define LAPIC_TIMER_DIV_128 0x0A /**< Teiler 128 */

/** @brief IDT-Vektor für den LAPIC-Timer-Interrupt */
#define LAPIC_TIMER_VECTOR 0xFE

/* I/O-APIC-Register-Offsets */
#define IOAPIC_REG_INDEX 0x00                        /**< Register-Index-Auswahl */
#define IOAPIC_REG_DATA 0x10                         /**< Daten-Register */
#define IOAPIC_REG_ID 0x00                           /**< I/O APIC ID Register */
#define IOAPIC_REG_VER 0x01                          /**< I/O APIC Versions-Register */
#define IOAPIC_REG_ARB 0x02                          /**< Bus-Arbitrierungs-ID */
#define IOAPIC_REG_RED_TABLE(idx) (0x10 + (idx) * 2) /**< Berechnung des Redirection-Table-Eintrags */

/* Inter-Processor Interrupt (IPI) Vektoren */
#define IPI_RESCHEDULE_VECTOR 0xFD    /**< Vektor für Rescheduling-IPI */
#define IPI_STOP_VECTOR 0xFC          /**< Vektor zum Anhalten anderer Kerne */
#define IPI_TLB_SHOOTDOWN_VECTOR 0xFB /**< Vektor für TLB-Flush auf anderen Kernen */

/**
 * @brief Initialisiert das APIC-Subsystem (LAPIC und I/O-APIC).
 */
void apic_init();

/**
 * @brief Schreibt einen 32-Bit-Wert in ein Register des Local APIC.
 * @param reg Register-Offset im LAPIC-MMIO-Bereich.
 * @param val Der zu schreibende Wert.
 */
void lapic_write(uint32_t reg, uint32_t val);

/**
 * @brief Liest einen 32-Bit-Wert aus einem Register des Local APIC.
 * @param reg Register-Offset im LAPIC-MMIO-Bereich.
 * @return Der ausgelesene 32-Bit-Wert.
 */
uint32_t lapic_read(uint32_t reg);

/**
 * @brief Sendet das End-Of-Interrupt-Signal (EOI) an den Local APIC.
 */
void lapic_eoi();

/**
 * @brief Routet einen externen Hardware-Interrupt (IRQ) auf einen IDT-Vektor.
 * @param irq ISA-IRQ-Nummer oder GSI-Pin.
 * @param vector Ziel-IDT-Vektor.
 * @param cpu_id ID des Ziel-Prozessorkerns.
 */
void ioapic_route_irq(uint8_t irq, uint8_t vector, uint8_t cpu_id);

/**
 * @brief Sendet ein INIT-IPI an einen spezifischen Prozessorkern.
 * @param lapic_id Hardware-APIC-ID des Zielkerns.
 */
void lapic_send_init(uint32_t lapic_id);

/**
 * @brief Sendet ein Startup-IPI (SIPI) an einen spezifischen Prozessorkern.
 * @param lapic_id Hardware-APIC-ID des Zielkerns.
 * @param vector Seitennummer in der untersten 1MB-Region (z. B. 0x08 für 0x8000).
 */
void lapic_send_sipi(uint32_t lapic_id, uint8_t vector);

/**
 * @brief Ermittelt die APIC-ID des aktuell ausführenden Prozessorkerns.
 * @return Die Hardware-APIC-ID des aktuellen Kerns.
 */
uint32_t lapic_get_id();

/**
 * @brief Sendet einen Reschedule-IPI-Broadcast an alle anderen Prozessorkerne.
 */
void lapic_send_broadcast_reschedule_ipi();

/**
 * @brief Sendet einen Stop-IPI-Broadcast an alle anderen Prozessorkerne, um diese anzuhalten.
 */
void lapic_send_broadcast_stop_ipi();

/**
 * @brief Sendet einen TLB-Shootdown-IPI-Broadcast an alle anderen Prozessorkerne.
 */
void lapic_send_broadcast_tlb_ipi();

/**
 * @brief Kalibriert den LAPIC-Timer mithilfe des HPET und startet ihn im periodischen Modus.
 * @param target_hz Gewünschte Interrupt-Frequenz in Hertz (z. B. 1000 für 1 kHz).
 */
void lapic_timer_calibrate_and_start(uint32_t target_hz);

/**
 * @brief Startet den LAPIC-Timer auf einem Application Processor (AP) mit den zuvor kalibrierten Werten.
 */
void lapic_timer_start_ap();

/** @brief Kalibrierter Startwert für den LAPIC-Timer-Initialzähler */
extern uint32_t lapic_timer_calibrated_initcnt;

/** @brief Ziel-Frequenz des LAPIC-Timers in Hz */
extern uint32_t lapic_timer_target_hz;

/** @brief Status der APIC-Initialisierung */
extern bool apic_initialized;

/**
 * @brief Prüft, ob das APIC-Subsystem bereits initialisiert wurde.
 * @return `true` wenn initialisiert, sonst `false`.
 */
static inline bool is_apic_initialized() {
    return apic_initialized;
}