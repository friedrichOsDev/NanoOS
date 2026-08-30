/**
 * @file thread.h
 * @brief Thread management and TCB definition
 * @author friedrichOsDev
 */

#pragma once

#include <core/process.h>
#include <core/sync.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_SIZE (16 * 1024) // 16 KiB Kernel Stack per Thread
#define DEFAULT_TIME_SLICE 10  // 10 Ticks

typedef enum {
    THREAD_EMBRYO,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_SLEEPING,
    THREAD_DEAD
} thread_state_t;

typedef void (*thread_entry_t)(void *arg);

typedef struct thread {
    uint64_t tid;
    char name[32];
    thread_state_t state;

    process_t *process;

    void *kernel_stack;
    uint64_t kernel_stack_top; // For TSS rsp0 in Ring 3
    uint64_t rsp;              // Current Stack Pointer

    uint64_t time_slice;       // Remaining Ticks for this thread
    uint64_t sleep_until_tick; // For sleep functionality
    int cpu_affinity;          // CPU Core ID (-1 if doesn't matter)

    // FPU/SSE State
    uint8_t fpu_state[512] __attribute__((aligned(16)));

    struct thread *next;
    struct thread *prev;

    struct thread *proc_next;
} thread_t;

thread_t *thread_create(process_t *proc, thread_entry_t entry, void *arg,
                        const char *name);
thread_t *thread_create_on_cpu(process_t *proc, thread_entry_t entry, void *arg,
                               const char *name, int cpu_affinity);
void thread_exit(void);