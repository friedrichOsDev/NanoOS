/**
 * @file hpet.h
 * @brief Treiber-Schnittstelle für den High Precision Event Timer (HPET).
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/**
 * @brief Repräsentiert die Register eines einzelnen HPET-Timers / Comparators.
 */
typedef struct {
    uint64_t configuration_and_capability; /**< Konfigurations- und Leistungseigenschaften des Timers */
    uint64_t comparator_value;             /**< Comparator-Wert für Interrupt-Auslösung */
    uint64_t fsb_interrupt_route;          /**< FSB/MSI Interrupt-Routing-Konfiguration */
    uint64_t reserved;                     /**< Reserviertes Feld */
} __attribute__((packed)) hpet_timer_t;

/**
 * @brief Struktur zur Abbildung des MMIO-Registerblocks des HPET.
 */
typedef struct {
    uint64_t general_capabilities;     /**< Allgemeine Leistungsmerkmale und Periodenlänge (in Femtosekunden) */
    uint64_t reserved0;                /**< Reserviert */
    uint64_t general_configuration;    /**< Allgemeine HPET-Konfiguration (Enable/Disable, Legacy Replacement) */
    uint64_t reserved1;                /**< Reserviert */
    uint64_t general_interrupt_status; /**< Interrupt-Status aller Timer */
    uint8_t reserved2[200];            /**< Reservierter Offset-Bereich bis zum Main Counter */
    uint64_t main_counter_value;       /**< Aktueller Wert des HPET-Hauptzählers */
    uint64_t reserved3;                /**< Reserviert */

    hpet_timer_t timers[]; /**< Flexibles Array der verfügbaren HPET-Timer */
} __attribute__((packed)) hpet_registers_t;

/** @brief Zeiger auf den per MMIO gemappten Registerblock des HPET. */
extern volatile hpet_registers_t *hpet_regs;

/** @brief Berechnete Anzahl von Zählerticks pro Mikrosekunde. */
extern uint64_t hpet_ticks_per_us;

/** @brief Berechnete Anzahl von Zählerticks pro Millisekunde. */
extern uint64_t hpet_ticks_per_ms;

/** @brief Hauptfrequenz des HPET-Counters in Hertz (Hz). */
extern uint64_t hpet_frequency_hz;
