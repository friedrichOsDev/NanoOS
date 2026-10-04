/**
 * @file smp.h
 * @brief Symmetric Multiprocessing (SMP) – Schnittstelle und Kernstrukturen.
 * @details Verwaltet Mehrkern-CPUs, pro-Prozessor-Strukturen (Per-CPU-Data) und
 *          bietet Hilfsfunktionen zur Bestimmung des aktuellen CPU-Cores.
 * @author friedrichOsDev
 */

#pragma once

#include <core/thread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Maximale Anzahl von CPU-Kernen, die das System unterstützt. */
#define MAX_CPUS 64

/**
 * @struct cpu_local
 * @brief Pro-CPU Speicherkontext zur Speicherung lokaler Kern-Zustände.
 */
typedef struct cpu_local {
    uint32_t cpu_id;             /**< Logische CPU-Nummer (0 = BSP, 1 = AP1, ...) */
    uint32_t lapic_id;           /**< Physikalische Local-APIC-ID aus der ACPI MADT */
    uint64_t kernel_stack;       /**< Basisadresse des Kernel-Stacks dieser CPU */
    volatile bool online;        /**< Flag: Gibt an, ob der CPU-Kern aktiv ist */
    thread_t *current_thread;    /**< Zeiger auf den aktuell auf dieser CPU laufenden Thread */
    thread_t *idle_thread;       /**< Dedizierter Idle-Thread für diese CPU */
    thread_t *thread_to_enqueue; /**< Temporärer Thread-Puffer für Scheduler-Einreihungen */
} cpu_local_t;

/** @brief Globales Array aller registrierten CPU-Kontexte im System. */
extern cpu_local_t cpus[MAX_CPUS];

/** @brief Gesamtzahl der erkannten und registrierten CPU-Kerne. */
extern size_t smp_cpu_count;

/**
 * @brief Scanned die ACPI MADT, registriert alle verfügbaren Kerne und führt die INIT-SIPI-SIPI Sequenz durch.
 */
void smp_init();

/**
 * @brief Ermittelt die Pro-CPU-Struktur des aktuell ausführenden CPU-Kerns über die LAPIC-ID.
 * @return Zeiger auf die `cpu_local_t`-Struktur des Kerns oder NULL, falls nicht gefunden.
 */
cpu_local_t *smp_get_current_cpu();