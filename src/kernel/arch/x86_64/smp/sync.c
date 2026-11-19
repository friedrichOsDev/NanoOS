/**
 * @file sync.c
 * @brief Thread synchronization primitives (spinlock, mutex) implementation.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <kernel/sync.h>
#include <pch.h>
#include <sched/sched.h>
#include <sched/thread.h>

/**
 * @brief Acquires a spinlock, spinning with a pause hint until it becomes available.
 *
 * @param[in,out] lock Pointer to the spinlock to acquire.
 */
void spinlock_acquire(spinlock_t *lock) {
    uint32_t expected = 1;
    while (__atomic_exchange_n(&(lock->lock), expected, __ATOMIC_ACQUIRE)) {
        while (__atomic_load_n(&(lock->lock), __ATOMIC_RELAXED)) {
            arch_pause();
        }
    }
}

/**
 * @brief Releases a spinlock.
 *
 * @param[in,out] lock Pointer to the spinlock to release.
 */
void spinlock_release(spinlock_t *lock) {
    __atomic_clear(&(lock->lock), __ATOMIC_RELEASE);
}

/**
 * @brief Saves the current interrupt flags, disables CPU interrupts, and acquires a spinlock.
 *
 * @param[in,out] lock Pointer to the spinlock to acquire.
 * @return Previous CPU interrupt state flags.
 */
irq_flags_t spinlock_acquire_irqsave(spinlock_t *lock) {
    irq_flags_t rflags = arch_irq_save();
    spinlock_acquire(lock);
    return rflags;
}

/**
 * @brief Releases a spinlock and restores the previous CPU interrupt state flags.
 *
 * @param[in,out] lock Pointer to the spinlock to release.
 * @param[in] rflags Interrupt state flags to restore.
 */
void spinlock_release_irqrestore(spinlock_t *lock, irq_flags_t rflags) {
    spinlock_release(lock);
    arch_irq_restore(rflags);
}

/**
 * @brief Acquires a blocking mutex, blocking the current thread and invoking the scheduler if already locked.
 *
 * @param[in,out] mux Pointer to the mutex to acquire.
 */
void mutex_lock(mutex_t *mux) {
    if (!mux)
        return;
    irq_flags_t flags = spinlock_acquire_irqsave(&mux->lock);

    thread_t *current = sched_get_current_thread();
    thread_t *owner = mux->owner;
    bool recursive = current && owner == current;

    if (!mux->locked) {
        mux->locked = true;
        mux->owner = current;
        spinlock_release_irqrestore(&mux->lock, flags);
        return;
    }
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

    if (recursive) {
        log_error("mutex recursive lock: %s (tid=%lu)",
                  mux->name ? mux->name : "<unnamed>", current->tid);
    }
    log_debug("mutex contention: %s waiter=%s(tid=%lu) owner=%s(tid=%lu)",
              mux->name ? mux->name : "<unnamed>", current->name, current->tid,
              owner ? owner->name : "<unknown>", owner ? owner->tid : 0UL);
    log_debug("mutex blocked: %s thread=%s(tid=%lu)",
              mux->name ? mux->name : "<unnamed>", current->name, current->tid);
    sched_schedule();
    log_debug("mutex resumed: %s thread=%s(tid=%lu)",
              mux->name ? mux->name : "<unnamed>", current->name, current->tid);
}

/**
 * @brief Releases a mutex, waking up the next thread in the wait queue or clearing the lock state.
 *
 * @param[in,out] mux Pointer to the mutex to release.
 */
void mutex_unlock(mutex_t *mux) {
    if (!mux)
        return;
    thread_t *current = sched_get_current_thread();
    thread_t *waiter = NULL;
    thread_t *owner = NULL;
    bool non_owner = false;
    irq_flags_t flags = spinlock_acquire_irqsave(&mux->lock);

    owner = mux->owner;
    non_owner = current && owner && owner != current;

    if (mux->wait_queue != NULL) {
        waiter = (thread_t *)mux->wait_queue;
        mux->wait_queue = (struct thread *)waiter->next;
        waiter->next = NULL;
        waiter->state = THREAD_READY;
        mux->owner = waiter; /* direct handoff; lock remains held */
    } else {
        mux->locked = false;
        mux->owner = NULL;
    }

    spinlock_release_irqrestore(&mux->lock, flags);

    if (non_owner) {
        log_error("mutex unlock by non-owner: %s caller=%s(tid=%lu) owner=%s(tid=%lu)",
                  mux->name ? mux->name : "<unnamed>", current->name, current->tid,
                  owner->name, owner->tid);
    }

    /* Avoid acquiring sched_lock while holding mux->lock. */
    if (waiter) {
        log_debug("mutex handoff: %s -> %s(tid=%lu)",
                  mux->name ? mux->name : "<unnamed>", waiter->name, waiter->tid);
        sched_add_thread(waiter);
    }
}

/**
 * @brief Attempts to acquire a mutex without blocking if it is currently locked.
 *
 * @param[in,out] mux Pointer to the mutex to try.
 * @return Returns true if successfully acquired, false otherwise.
 */
bool mutex_trylock(mutex_t *mux) {
    if (!mux)
        return false;

    irq_flags_t flags = spinlock_acquire_irqsave(&mux->lock);

    if (!mux->locked) {
        mux->locked = true;
        mux->owner = sched_get_current_thread();
        spinlock_release_irqrestore(&mux->lock, flags);
        return true;
    }

    spinlock_release_irqrestore(&mux->lock, flags);
    return false;
}
