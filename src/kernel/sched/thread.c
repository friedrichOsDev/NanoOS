/**
 * @file thread.c
 * @brief Thread initialization and creation.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <kernel/panic.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <pch.h>
#include <sched/process.h>
#include <sched/sched.h>
#include <sched/thread.h>

static uint64_t next_tid = 1;
static spinlock_t tid_lock = SPINLOCK_INIT;

/**
 * @brief Creates a new thread assigned to a specific CPU core.
 *
 * @param[in] proc Parent process (defaults to kernel process if NULL).
 * @param[in] entry Thread entry point function.
 * @param[in] arg Argument pointer passed to the thread entry function.
 * @param[in] name Debugging name of the thread.
 * @param[in] cpu_affinity Target CPU core ID (-1 for any core).
 * @param[out] out Pointer to store the created thread structure.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t thread_create_on_cpu(process_t *proc, thread_entry_t entry, void *arg, const char *name, int cpu_affinity, thread_t **out) {
    if (cpu_affinity >= 0 && (size_t)cpu_affinity >= smp_get_cpu_count()) {
        log_error("cannot create thread on a non-existing core %d", cpu_affinity);
        return E_NOTAVAIL;
    }

    if (cpu_affinity == -1) {
        if (smp_get_cpu_count() <= 1) {
            cpu_affinity = 0;
        }
    }

    virt_addr_t mem;
    K_TRY(kzalloc(sizeof(thread_t), &mem)); // Returns err on failure
    thread_t *thread = (thread_t *)mem;

    irq_flags_t flags = spinlock_acquire_irqsave(&tid_lock);
    thread->tid = next_tid++;
    spinlock_release_irqrestore(&tid_lock, flags);

    if (name) {
        strncpy(thread->name, name, sizeof(thread->name) - 1);
    } else {
        strncpy(thread->name, "unnamed", sizeof(thread->name) - 1);
    }

    thread->process = proc ? proc : proc_get_kernel_proc();
    thread->state = THREAD_EMBRYO;
    thread->time_slice = DEFAULT_TIME_SLICE;
    thread->cpu_affinity = cpu_affinity;
    fpu_state_init(thread->fpu_state);

    // Allocate the per-thread kernel stack (STACK_SIZE).
    kerr_t err = kzalloc(STACK_SIZE, &mem);
    if (k_failed(err)) {
        K_TRY(kfree((virt_addr_t)thread)); // Returns err on failure
        return err;
    }
    thread->kernel_stack = (void *)mem;
    thread->kernel_stack_top = (uint64_t)thread->kernel_stack + STACK_SIZE;

    uint64_t *sp = (uint64_t *)thread->kernel_stack_top;

    sp = (uint64_t *)((uint64_t)sp & ~0xFULL);
    *(--sp) = (uint64_t)thread_entry_stub;

    // Registers for switch_context (pop rbp, rbx, r12, r13, r14, r15)
    *(--sp) = (uint64_t)0;     // RBP
    *(--sp) = (uint64_t)0;     // RBX
    *(--sp) = (uint64_t)entry; // R12 (Function Pointer)
    *(--sp) = (uint64_t)arg;   // R13 (Argument)
    *(--sp) = (uint64_t)0;     // R14
    *(--sp) = (uint64_t)0;     // R15

    thread->rsp = (uint64_t)sp;
    k_assert(thread->rsp % 16 == 8);
    thread->state = THREAD_READY;

    irq_flags_t proc_flags = spinlock_acquire_irqsave(&thread->process->lock);
    thread->proc_next = thread->process->threads;
    thread->process->threads = thread;
    thread->process->thread_count++;
    spinlock_release_irqrestore(&thread->process->lock, proc_flags);

    sched_add_thread(thread);

    log_trace("create thread with name: %s, tid: %d, core: %d, proc: %s", thread->name, thread->tid, thread->cpu_affinity, thread->process->name);

    *out = thread;
    return E_OK;
}

/**
 * @brief Creates a new thread with automatic CPU affinity.
 *
 * @param[in] proc Parent process (defaults to kernel process if NULL).
 * @param[in] entry Thread entry point function.
 * @param[in] arg Argument pointer passed to the thread entry function.
 * @param[in] name Debugging name of the thread.
 * @param[out] out Pointer to store the created thread structure.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t thread_create(process_t *proc, thread_entry_t entry, void *arg, const char *name, thread_t **out) {
    return thread_create_on_cpu(proc, entry, arg, name, -1, out);
}

/**
 * @brief Destroys a thread, removes it from its parent process, and frees its stack and memory.
 *
 * @param[in,out] thread Pointer to the thread to destroy.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t thread_destroy(thread_t *thread) {
    if (!thread) {
        return E_INVAL;
    }

    if (thread == sched_get_current_thread() || thread->state == THREAD_RUNNING) {
        log_error("cannot destroy a currently running thread (tid: %lu)", thread->tid);
        return E_NOTAVAIL;
    }

    if (thread->process) {
        irq_flags_t flags = spinlock_acquire_irqsave(&thread->process->lock);
        thread_t **pp = &thread->process->threads;
        while (*pp) {
            if (*pp == thread) {
                *pp = thread->proc_next;
                if (thread->process->thread_count > 0) {
                    thread->process->thread_count--;
                }
                break;
            }
            pp = &(*pp)->proc_next;
        }
        spinlock_release_irqrestore(&thread->process->lock, flags);
        thread->proc_next = NULL;
    }

    if (thread->kernel_stack) {
        K_TRY(kfree((virt_addr_t)thread->kernel_stack));
        thread->kernel_stack = NULL;
    }

    log_trace("destroy thread with name: %s, tid: %lu", thread->name, thread->tid);

    return kfree((virt_addr_t)thread);
}
