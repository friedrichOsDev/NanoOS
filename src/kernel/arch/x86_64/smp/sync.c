/**
 * @file sync.c
 * @brief Implementierung der Primitiven für Spinlocks und Mutexes.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <kernel/sync.h>
#include <pch.h>
#include <sched/sched.h>
#include <sched/thread.h>

void spinlock_acquire(spinlock_t *lock) {
    uint32_t expected = 1;
    while (__atomic_exchange_n(&(lock->lock), expected, __ATOMIC_ACQUIRE)) {
        while (__atomic_load_n(&(lock->lock), __ATOMIC_RELAXED)) {
            arch_pause();
        }
    }
}

void spinlock_release(spinlock_t *lock) {
    __atomic_clear(&(lock->lock), __ATOMIC_RELEASE);
}

irq_flags_t spinlock_acquire_irqsave(spinlock_t *lock) {
    irq_flags_t rflags = arch_irq_save();
    spinlock_acquire(lock);
    return rflags;
}

void spinlock_release_irqrestore(spinlock_t *lock, irq_flags_t rflags) {
    spinlock_release(lock);
    arch_irq_restore(rflags);
}

void mutex_lock(mutex_t *mux) {
    if (!mux)
        return;
    irq_flags_t flags = spinlock_acquire_irqsave(&mux->lock);

    if (!mux->locked) {
        mux->locked = true;
        spinlock_release_irqrestore(&mux->lock, flags);
        return;
    }

    thread_t *current = sched_get_current_thread();
    if (!current) {
        spinlock_release_irqrestore(&mux->lock, flags);
        return;
    }

    current->state = THREAD_BLOCKED;

    current->next = NULL;
    if (mux->wait_queue == NULL) {
        mux->wait_queue = (struct thread *)current;
    } else {
        thread_t *temp = (thread_t *)mux->wait_queue;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = current;
    }

    spinlock_release_irqrestore(&mux->lock, flags);

    sched_schedule();
}

void mutex_unlock(mutex_t *mux) {
    if (!mux)
        return;
    irq_flags_t flags = spinlock_acquire_irqsave(&mux->lock);

    if (mux->wait_queue != NULL) {
        thread_t *waiter = (thread_t *)mux->wait_queue;
        mux->wait_queue = (struct thread *)waiter->next;

        waiter->next = NULL;
        waiter->state = THREAD_READY;

        sched_add_thread(waiter);
    } else {
        mux->locked = false;
    }

    spinlock_release_irqrestore(&mux->lock, flags);
}

bool mutex_trylock(mutex_t *mux) {
    if (!mux)
        return false;

    irq_flags_t flags = spinlock_acquire_irqsave(&mux->lock);

    if (!mux->locked) {
        mux->locked = true;
        spinlock_release_irqrestore(&mux->lock, flags);
        return true;
    }

    spinlock_release_irqrestore(&mux->lock, flags);
    return false;
}
