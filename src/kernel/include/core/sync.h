/**
 * @file sync.h
 * @brief Spinlocks and synchronization primitives
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

#define SPINLOCK_INIT ((spinlock_t){.lock = 0})

static inline void spinlock_init(spinlock_t *lock) { lock->lock = 0; }

void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

uint64_t spinlock_acquire_irqsave(spinlock_t *lock);
void spinlock_release_irqrestore(spinlock_t *lock, uint64_t rflags);