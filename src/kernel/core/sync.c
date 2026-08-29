/**
 * @file sync.c
 * @brief Spinlock implementation
 * @author friedrichOsDev
 */

#include <core/sync.h>

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
    while (__atomic_test_and_set(&(lock->lock), __ATOMIC_ACQUIRE)) {
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