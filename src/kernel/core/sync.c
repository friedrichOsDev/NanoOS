/**
 * @file sync.c
 * @brief Spinlock implementation
 * @author friedrichOsDev
 */

#include <core/scheduler.h>
#include <core/sync.h>
#include <core/thread.h>

static inline uint64_t read_rflags_and_cli(void) {
    uint64_t rflags;
    __asm__ __volatile__("pushfq\n\t"
                         "pop %0\n\t"
                         "cli"
                         : "=r"(rflags)
                         :
                         : "memory");
    return rflags;
}

static inline void restore_rflags(uint64_t rflags) {
    __asm__ __volatile__("pushq %0\n\t"
                         "popfq"
                         :
                         : "r"(rflags)
                         : "memory", "cc");
}

void spinlock_acquire(spinlock_t *lock) {
    uint32_t expected = 1;
    while (__atomic_exchange_n(&(lock->lock), expected, __ATOMIC_ACQUIRE)) {
        while (__atomic_load_n(&(lock->lock), __ATOMIC_RELAXED)) {
            __asm__ __volatile__("pause");
        }
    }
}

void spinlock_release(spinlock_t *lock) {
    __atomic_clear(&(lock->lock), __ATOMIC_RELEASE);
}

uint64_t spinlock_acquire_irqsave(spinlock_t *lock) {
    uint64_t rflags = read_rflags_and_cli();
    spinlock_acquire(lock);
    return rflags;
}

void spinlock_release_irqrestore(spinlock_t *lock, uint64_t rflags) {
    spinlock_release(lock);
    restore_rflags(rflags);
}

void mutex_lock(mutex_t *mux) {
    uint64_t flags = spinlock_acquire_irqsave(&mux->lock);

    if (!mux->locked) {
        mux->locked = true;
        spinlock_release_irqrestore(&mux->lock, flags);
        return;
    }

    thread_t *current = scheduler_get_current_thread();
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

    spinlock_release(&mux->lock);

    scheduler_schedule();
}

void mutex_unlock(mutex_t *mux) {
    uint64_t flags = spinlock_acquire_irqsave(&mux->lock);

    if (mux->wait_queue != NULL) {
        thread_t *waiter = (thread_t *)mux->wait_queue;
        mux->wait_queue = (struct thread *)waiter->next;

        waiter->next = NULL;
        waiter->state = THREAD_READY;

        scheduler_add_thread(waiter);
    } else {
        mux->locked = false;
    }

    spinlock_release_irqrestore(&mux->lock, flags);
}