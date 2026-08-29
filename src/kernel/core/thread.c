/**
 * @file thread.c
 * @brief Thread management and creation
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/context.h>
#include <arch/x86_64/mm/heap.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <lib/string.h>

static uint64_t next_tid = 1;
static spinlock_t tid_lock = SPINLOCK_INIT;

thread_t *thread_create(process_t *proc, thread_entry_t entry, void *arg,
                        const char *name) {
    thread_t *thread = (thread_t *)kzalloc(sizeof(thread_t));
    if (!thread)
        return NULL;

    uint64_t flags = spinlock_acquire_irqsave(&tid_lock);
    thread->tid = next_tid++;
    spinlock_release_irqrestore(&tid_lock, flags);

    if (name) {
        strncpy(thread->name, name, sizeof(thread->name) - 1);
    } else {
        strncpy(thread->name, "unnamed", sizeof(thread->name) - 1);
    }

    thread->process = proc ? proc : kernel_process;
    thread->state = THREAD_EMBRYO;
    thread->time_slice = DEFAULT_TIME_SLICE;
    thread->cpu_affinity = -1;

    // allocate stack (16 KiB)
    thread->kernel_stack = (void *)kmalloc(STACK_SIZE);
    if (!thread->kernel_stack) {
        kfree((virt_addr_t)thread);
        return NULL;
    }

    thread->kernel_stack_top = (uint64_t)thread->kernel_stack + STACK_SIZE;

    // initialize stack frame for context switch
    // [thread_entry_stub]  <- destination for ret
    // [r15]
    // [r14]
    // [r13 = arg]          <- args for thread_entry_stub
    // [r12 = entry]        <- pointer to thread_entry_stub
    // [rbp = 0]
    // [rbx = 0]            <- Hierhin zeigt thread->rsp initial

    uint64_t *sp = (uint64_t *)thread->kernel_stack_top;

    sp = (uint64_t *)((uint64_t)sp & ~0xFULL);

    *(--sp) = (uint64_t)thread_entry_stub;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = (uint64_t)entry;
    *(--sp) = (uint64_t)arg;
    *(--sp) = 0;
    *(--sp) = 0;

    thread->rsp = (uint64_t)sp;
    thread->state = THREAD_READY;

    scheduler_add_thread(thread);

    return thread;
}

void thread_exit(void) { scheduler_thread_exit(); }