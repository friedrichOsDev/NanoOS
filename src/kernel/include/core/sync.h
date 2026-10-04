/**
 * @file sync.h
 * @brief Implementierung von Spinlocks und Mutexes für die Kernel-Synchronisation.
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct thread;

/**
 * @struct spinlock_t
 * @brief Struktur für ein einfaches Atomic-Spinlock.
 */
typedef struct {
    uint32_t lock; /**< Sperr-Zustand (0 = frei, 1 = belegt). */
} spinlock_t;

/**
 * @struct mutex_t
 * @brief Struktur für ein blockierendes Mutex mit Warteschlange.
 */
typedef struct {
    spinlock_t lock;           /**< Interner Spinlock zum Schutz der Mutex-Struktur. */
    volatile bool locked;      /**< Sperr-Status des Mutex (true = belegt). */
    struct thread *wait_queue; /**< Zeiger auf die Liste der wartenden Threads. */
    const char *name;          /**< Name des Mutex für Debugging-Zwecke. */
} mutex_t;

/**
 * @brief Initialisierungs-Makro für statische Spinlocks.
 */
#define SPINLOCK_INIT ((spinlock_t){.lock = 0})

/**
 * @brief Initialisiert einen Spinlock-Zustand inline.
 * @param lock Zeiger auf die zu initialisierende Spinlock-Struktur.
 */
static inline void spinlock_init(spinlock_t *lock) { lock->lock = 0; }

/**
 * @brief Fordert einen Spinlock an (Busy-Waiting).
 * @param lock Zeiger auf den Spinlock.
 */
void spinlock_acquire(spinlock_t *lock);

/**
 * @brief Gibt einen Spinlock wieder frei.
 * @param lock Zeiger auf den Spinlock.
 */
void spinlock_release(spinlock_t *lock);

/**
 * @brief Deaktiviert Interrupts (CLI) und fordert einen Spinlock an.
 * @param lock Zeiger auf den Spinlock.
 * @return Der ursprüngliche RFLAGS-Registerwert vor dem Deaktivieren der Interrupts.
 */
uint64_t spinlock_acquire_irqsave(spinlock_t *lock);

/**
 * @brief Gibt einen Spinlock frei und stellt den vorherigen Interrupt-Zustand wieder her.
 * @param lock Zeiger auf den Spinlock.
 * @param rflags Gespeicherter RFLAGS-Registerwert zur Wiederherstellung des Interrupt-Status.
 */
void spinlock_release_irqrestore(spinlock_t *lock, uint64_t rflags);

/**
 * @brief Initialisiert ein Mutex-Objekt.
 * @param mux Zeiger auf das zu initialisierende Mutex.
 * @param name Debug-Bezeichnung für das Mutex.
 */
static inline void mutex_init(mutex_t *mux, const char *name) {
    spinlock_init(&mux->lock);
    mux->locked = false;
    mux->wait_queue = NULL;
    mux->name = name;
}

/**
 * @brief Sperrt ein Mutex. Blockiert den aufrufenden Thread, falls das Mutex belegt ist.
 * @param mux Zeiger auf das Mutex.
 */
void mutex_lock(mutex_t *mux);

/**
 * @brief Entsperrt ein Mutex und weckt den nächsten wartenden Thread in der Warteschlange auf.
 * @param mux Zeiger auf das Mutex.
 */
void mutex_unlock(mutex_t *mux);