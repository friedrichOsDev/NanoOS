/**
 * @file sync.h
 * @brief Implementierung von Spinlocks und Mutexes für die Kernel-Synchronisation.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

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
    struct thread *owner;           /**< Referenz auf den Thread, der den Mutex besitzt. */
    volatile bool locked;      /**< Sperr-Status des Mutex (true = belegt). */
    struct thread *wait_queue; /**< Zeiger auf die Liste der wartenden Threads. */
    const char *name;          /**< Name des Mutex für Debugging-Zwecke. */
} mutex_t;

#define SPINLOCK_INIT \
    { .lock = 0 }

#define MUTEX_INIT(n) \
    { .name = (n) }

inline void spinlock_init(spinlock_t *lock) { lock->lock = 0; }
inline void mutex_init(mutex_t *mux, const char *name) {
    spinlock_init(&mux->lock);
    mux->locked = false;
    mux->wait_queue = NULL;
    mux->name = name;
}

void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);
irq_flags_t spinlock_acquire_irqsave(spinlock_t *lock);
void spinlock_release_irqrestore(spinlock_t *lock, irq_flags_t rflags);

void mutex_lock(mutex_t *mux);
void mutex_unlock(mutex_t *mux);
bool mutex_trylock(mutex_t *mux);
