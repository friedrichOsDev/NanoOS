/**
 * @file sync.h
 * @brief Spinlocks and synchronization primitives
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct thread;

typedef struct {
    uint32_t lock;
} spinlock_t;

typedef struct {
    spinlock_t lock;
    volatile bool locked;
    struct thread *wait_queue;
    const char *name;
} mutex_t;

#define SPINLOCK_INIT ((spinlock_t){.lock = 0})

static inline void spinlock_init(spinlock_t *lock) { lock->lock = 0; }

void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);
uint64_t spinlock_acquire_irqsave(spinlock_t *lock);
void spinlock_release_irqrestore(spinlock_t *lock, uint64_t rflags);

static inline void mutex_init(mutex_t *mux, const char *name) {
    spinlock_init(&mux->lock);
    mux->locked = false;
    mux->wait_queue = NULL;
    mux->name = name;
}

void mutex_lock(mutex_t *mux);
void mutex_unlock(mutex_t *mux);