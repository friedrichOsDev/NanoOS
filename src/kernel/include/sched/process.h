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

void proc_init();
process_t *proc_create(const char *name, vmm_space_t *sp);
void proc_register(process_t *proc);
process_t *proc_get_kernel_proc();
process_t *proc_get_list();
