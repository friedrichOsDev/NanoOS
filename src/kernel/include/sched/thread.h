/**
 * @file thread.h
 * @brief Thread definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <kernel/sync.h>
#include <pch.h>

#define STACK_SIZE (64 * 1024)
#define DEFAULT_TIME_SLICE 10

/** @brief States a thread can assume during its lifecycle. */
typedef enum {
    THREAD_EMBRYO,   /**< Thread is currently being initialized */
    THREAD_READY,    /**< Thread is ready to run in the scheduler */
    THREAD_RUNNING,  /**< Thread is currently executing on a CPU core */
    THREAD_BLOCKED,  /**< Thread is waiting on an event or lock */
    THREAD_SLEEPING, /**< Thread is sleeping for a specified duration */
    THREAD_DEAD      /**< Thread has terminated and awaits cleanup */
} thread_state_t;

typedef void (*thread_entry_t)(void *arg);

/** @brief Thread Control Block (TCB). */
typedef struct thread {
    uint64_t tid;                                        /**< Unique Thread Identifier (TID) */
    char name[32];                                       /**< Name of the thread for debugging purposes */
    thread_state_t state;                                /**< Current execution state of the thread */
    process_t *process;                                  /**< Pointer to the parent process */
    void *kernel_stack;                                  /**< Base pointer of the allocated kernel stack */
    uint64_t kernel_stack_top;                           /**< Top boundary of the stack (for TSS.rsp0 on Ring 3 transitions) */
    uint64_t rsp;                                        /**< Current stack pointer of the thread during context switches */
    uint64_t time_slice;                                 /**< Remaining execution ticks in the current time slice */
    uint64_t sleep_until_tick;                           /**< Target tick count for time-based sleep states */
    int cpu_affinity;                                    /**< Preferred CPU core ID (-1 for any core) */
    uint8_t fpu_state[512] __attribute__((aligned(16))); /**< 16-byte aligned storage for FPU/SSE/AVX registers */
    struct thread *next;                                 /**< Pointer to the next thread in the scheduler list */
    struct thread *prev;                                 /**< Pointer to the previous thread in the scheduler list */
    struct thread *proc_next;                            /**< Pointer to the next thread within the same process */
} __attribute__((aligned(16))) thread_t;

kerr_t thread_create(process_t *proc, thread_entry_t entry, void *arg, const char *name, thread_t **out);
kerr_t thread_create_on_cpu(process_t *proc, thread_entry_t entry, void *arg, const char *name, int cpu_affinity, thread_t **out);
kerr_t thread_destroy(thread_t *thread);
