/**
 * @file sync.h
 * @brief Thread synchronization primitives (spinlock, mutex) definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

struct thread;

/** @brief Structure for a simple atomic spinlock. */
typedef struct {
    uint32_t lock; /**< Lock state (0 = free, 1 = locked). */
} spinlock_t;

/** @brief Structure for a blocking mutex with a wait queue. */
typedef struct {
    spinlock_t lock;           /**< Internal spinlock protecting the mutex structure. */
    struct thread *owner;      /**< Reference to the thread that owns the mutex. */
    volatile bool locked;      /**< Lock status of the mutex (true = locked). */
    struct thread *wait_queue; /**< Pointer to the list of waiting threads. */
    const char *name;          /**< Name of the mutex for debugging purposes. */
} mutex_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"
#define SPINLOCK_INIT \
    { .lock = 0 }

#define MUTEX_INIT(n) \
    { .name = (n) }
#pragma GCC diagnostic pop

[[maybe_unused]] static inline void spinlock_init(spinlock_t *lock) { lock->lock = 0; }
[[maybe_unused]] static inline void mutex_init(mutex_t *mux, const char *name) {
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
