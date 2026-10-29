/**
 * @file process.h
 * @brief Definitionen und Datenstrukturen zur Prozessverwaltung.
 * @details Definiert die Struktur eines Prozesses, einschließlich Thread-Verwaltung,
 *          Adressraumzeiger (PML4/CR3) und Synchronisationselemente.
 * @author friedrichOsDev
 */

#pragma once

#include <kernel/sync.h>
#include <mm/memory.h>
#include <pch.h>

struct thread;

/**
 * @struct process
 * @brief Repräsentiert einen Prozess im System.
 */
typedef struct process {
    uint64_t pid;  /**< Eindeutige Prozess-ID (PID) */
    char name[32]; /**< Name des Prozesses */

    vmm_space_t *vmm_space; /**< Pointer zum Address-Bereich des Prozesses (incl: pml4, pml4_phys, lock) */

    struct thread *threads; /**< Zeiger auf die Liste der zugehörigen Threads */
    size_t thread_count;    /**< Anzahl der Threads im Prozess */

    spinlock_t lock;      /**< Spinlock zur Absicherung der Prozess-Datenstruktur */
    struct process *next; /**< Zeiger auf den nächsten Prozess in der globalen Prozessliste */
} process_t;

/** @brief Globaler Zeiger auf den Kernel-Prozess. */
extern process_t *kernel_process;

/** @brief Globaler Zeiger auf den Anfang der Prozessliste. */
extern process_t *proc_list;

/**
 * @brief Initialisiert das Prozess-Subsystem und erstellt den initialen Kernel-Prozess.
 */
void process_init();

/**
 * @brief Erstellt einen neuen Prozess mit einem angegebenen Namen und einer Seitentabelle.
 * @param name Der Name des neuen Prozesses.
 * @param pml4 Zeiger auf die virtuelle Adresse der PML4-Seitentabelle.
 * @return Zeiger auf die neu erstellte `process_t`-Struktur oder NULL bei Speicherallokationsfehlern.
 */
process_t *process_create(const char *name, vmm_space_t *sp);

/**
 * @brief Registriert einen bereits erstellten Prozess in der globalen Prozessliste.
 * @param proc Zeiger auf den zu registrierenden Prozess.
 */
void process_register(process_t *proc);
