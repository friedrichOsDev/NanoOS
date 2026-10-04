/**
 * @file process.h
 * @brief Definitionen und Datenstrukturen zur Prozessverwaltung.
 * @details Definiert die Struktur eines Prozesses, einschließlich Thread-Verwaltung,
 *          Adressraumzeiger (PML4/CR3) und Synchronisationselemente.
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <core/sync.h>
#include <stddef.h>
#include <stdint.h>

struct thread;

/**
 * @struct process
 * @brief Repräsentiert einen Prozess im System.
 */
typedef struct process {
    uint64_t pid;           /**< Eindeutige Prozess-ID (PID)[cite: 29] */
    char name[32];          /**< Name des Prozesses[cite: 29] */

    page_table_t *pml4;     /**< Virtuelle Adresse der PML4-Seitentabelle[cite: 29] */
    phys_addr_t cr3;        /**< Physikalische Adresse der PML4-Seitentabelle (CR3-Registerwert)[cite: 29] */

    struct thread *threads; /**< Zeiger auf die Liste der zugehörigen Threads[cite: 29] */
    size_t thread_count;    /**< Anzahl der Threads im Prozess[cite: 29] */

    spinlock_t lock;        /**< Spinlock zur Absicherung der Prozess-Datenstruktur[cite: 29] */
    struct process *next;   /**< Zeiger auf den nächsten Prozess in der globalen Prozessliste[cite: 29] */
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
process_t *process_create(const char *name, page_table_t *pml4);

/**
 * @brief Registriert einen bereits erstellten Prozess in der globalen Prozessliste.
 * @param proc Zeiger auf den zu registrierenden Prozess.
 */
void process_register(process_t *proc);