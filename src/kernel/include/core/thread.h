/**
 * @file thread.h
 * @brief Thread-Verwaltung und Definition des Thread Control Blocks (TCB).
 * @author friedrichOsDev
 */

#pragma once

#include <core/process.h>
#include <core/sync.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Größe des Kernel-Stacks pro Thread (16 KiB). */
#define STACK_SIZE (16 * 1024)

/** @brief Standard-Zeitscheibe für Threads in Ticks. */
#define DEFAULT_TIME_SLICE 10

/**
 * @enum thread_state_t
 * @brief Zustände, die ein Thread während seines Lebenszyklus annehmen kann.
 */
typedef enum {
    THREAD_EMBRYO,   /**< Thread wird gerade initialisiert. */
    THREAD_READY,    /**< Ausführungsbereit im Scheduler. */
    THREAD_RUNNING,  /**< Wird aktuell auf einer CPU ausgeführt. */
    THREAD_BLOCKED,  /**< Wartet auf ein Event oder eine Sperre. */
    THREAD_SLEEPING, /**< Schläft für eine bestimmte Zeitspanne. */
    THREAD_DEAD      /**< Beendet und wartet auf Bereinigung. */
} thread_state_t;

/**
 * @brief Funktionszeiger-Typ für den Einstiegspunkt eines Threads.
 * @param arg Beliebiger Argument-Zeiger, der dem Thread übergeben wird.
 */
typedef void (*thread_entry_t)(void *arg);

/**
 * @struct thread
 * @brief Thread Control Block (TCB).
 * @details Speichert den gesamten Zustand eines Threads inklusive Stacks, Registern und Affinität.
 */
typedef struct thread {
    uint64_t tid;         /**< Eindeutige Thread-ID. */
    char name[32];        /**< Name des Threads für Debugging-Zwecke. */
    thread_state_t state; /**< Aktueller Zustand des Threads. */

    process_t *process; /**< Zuordnungsverweis auf den Vaterprozess. */

    void *kernel_stack;        /**< Basiszeiger des zugewiesenen Kernel-Stacks. */
    uint64_t kernel_stack_top; /**< Obere Grenze des Stacks (für TSS.rsp0 bei Ring 3 Wecheln). */
    uint64_t rsp;              /**< Aktueller Stack-Pointer des Threads bei Kontextwechseln. */

    uint64_t time_slice;       /**< Verbleibende Ausführungs-Ticks in der aktuellen Zeitscheibe. */
    uint64_t sleep_until_tick; /**< Ziel-Tickwert für zeitbasierte Schlafzustände. */
    int cpu_affinity;          /**< Bevorzugter CPU-Kern ID (-1 bedeutet beliebiger Kern). */

    /** @brief Gespeicherter Puffer für FPU/SSE/AVX-Registerzustände (16-Byte ausgerichtet). */
    uint8_t fpu_state[512] __attribute__((aligned(16)));

    struct thread *next; /**< Zeiger auf den nächsten Thread in der Scheduler-Liste. */
    struct thread *prev; /**< Zeiger auf den vorherigen Thread in der Scheduler-Liste. */

    struct thread *proc_next; /**< Zeiger auf den nächsten Thread innerhalb desselben Prozesses. */
} __attribute__((aligned(16))) thread_t;

/**
 * @brief Erstellt einen neuen Thread mit automatischer CPU-Zuweisung.
 * @param proc Prozess, dem der Thread zugeordnet wird (falls NULL, wird `kernel_process` verwendet).
 * @param entry Einstiegsfunktion des Threads.
 * @param arg Übergabeparameter an die Einstiegsfunktion.
 * @param name Name des Threads.
 * @return Zeiger auf die neu erstellte `thread_t`-Struktur oder `NULL` bei Fehler.
 */
thread_t *thread_create(process_t *proc, thread_entry_t entry, void *arg, const char *name);

/**
 * @brief Erstellt einen neuen Thread mit expliziter CPU-Affinität.
 * @param proc Prozess, dem der Thread zugeordnet wird.
 * @param entry Einstiegsfunktion des Threads.
 * @param arg Übergabeparameter an die Einstiegsfunktion.
 * @param name Name des Threads.
 * @param cpu_affinity ID des Ziel-CPU-Kerns (-1 für beliebigen Kern).
 * @return Zeiger auf die neu erstellte `thread_t`-Struktur oder `NULL` bei Fehler.
 */
thread_t *thread_create_on_cpu(process_t *proc, thread_entry_t entry, void *arg, const char *name, int cpu_affinity);

/**
 * @brief Beendet den aktuell ausführenden Thread und übergibt die Kontrolle an den Scheduler.
 */
void thread_exit();