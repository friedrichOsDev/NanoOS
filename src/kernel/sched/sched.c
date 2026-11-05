/**
 * @file sched.c
 * @brief Preemptive round-robin scheduler implementation.
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

static thread_t *ready_queue_head = NULL;
static thread_t *ready_queue_tail = NULL;
static thread_t *sleep_queue_head = NULL;
static thread_t *dead_queue_head = NULL;

static spinlock_t sched_lock = SPINLOCK_INIT;
static volatile uint64_t system_ticks = 0;
static bool scheduler_enabled = false;

/** @brief Releases the initial scheduler lock after starting a thread. */
void scheduler_release_initial_lock(); // prevent -Wmissing-prototypes
void scheduler_release_initial_lock() { spinlock_release(&sched_lock); }

/**
 * @brief Idle thread routine executed when no other threads are ready.
 *
 * @param[in] arg Unused thread argument pointer.
 */
void idle_task(void *arg) {
    (void)arg;
    while (1) {
        arch_idle();
    }
}

/**
 * @brief Retrieves the currently executing thread on the local CPU core.
 *
 * @return Pointer to current thread, or NULL if none or SMP not ready.
 */
thread_t *sched_get_current_thread() {
    cpu_local_t *cpu = smp_get_current_cpu();
    return cpu ? cpu->current_thread : NULL;
}

/**
 * @brief Retrieves the system ticks counter since scheduler start.
 *
 * @return Current system tick count.
 */
uint64_t sched_get_ticks() { return system_ticks; }

/**
 * @brief Sets the CPU affinity for a specific thread.
 *
 * @param[in,out] thread Pointer to the thread.
 * @param[in] cpu_id Target CPU core ID, or -1 for any core.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t thread_set_affinity(thread_t *thread, int cpu_id) {
    if (!thread) {
        return E_INVAL;
    }

    if (cpu_id >= 0 && (size_t)cpu_id >= smp_get_cpu_count()) {
        log_error("cannot set thread affinity to non-existing core %d", cpu_id);
        return E_NOTAVAIL;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&sched_lock);
    thread->cpu_affinity = cpu_id;
    spinlock_release_irqrestore(&sched_lock, flags);
    return E_OK;
}

/**
 * @brief Appends a thread to the ready queue under locked scheduler state.
 *
 * @param[in,out] thread Pointer to the thread to enqueue.
 */
static void scheduler_add_thread_locked(thread_t *thread) {
    if (!thread)
        return;

    if (strncmp(thread->name, "idle", 4) == 0) {
        return;
    }

    thread->state = THREAD_READY;
    thread->next = NULL;
    if (ready_queue_tail) {
        ready_queue_tail->next = thread;
        thread->prev = ready_queue_tail;
        ready_queue_tail = thread;
    } else {
        ready_queue_head = thread;
        ready_queue_tail = thread;
        thread->prev = NULL;
    }

    if (smp_get_cpu_count() > 1) {
        arch_ipi_reschedule();
    }
}

/**
 * @brief Adds a thread to the scheduler's ready queue thread-safely.
 *
 * @param[in,out] thread Pointer to the thread to add.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t sched_add_thread(thread_t *thread) {
    if (!thread) {
        return E_INVAL;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&sched_lock);
    scheduler_add_thread_locked(thread);
    spinlock_release_irqrestore(&sched_lock, flags);
    return E_OK;
}

/**
 * @brief Dequeues the next ready thread suitable for execution on the given CPU core.
 *
 * @param[in] cpu_id CPU core ID requesting a thread.
 * @return Pointer to next ready thread, or NULL if no eligible thread is ready.
 */
thread_t *pop_next_ready_thread_for_cpu(int cpu_id) {
    thread_t *curr = ready_queue_head;

    while (curr) {
        if (curr->cpu_affinity == -1 || curr->cpu_affinity == cpu_id) {
            if (curr->prev) {
                curr->prev->next = curr->next;
            } else {
                ready_queue_head = curr->next;
            }

            if (curr->next) {
                curr->next->prev = curr->prev;
            } else {
                ready_queue_tail = curr->prev;
            }

            curr->next = NULL;
            curr->prev = NULL;
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

/** @brief Reaps and cleans up resources of terminated (dead) threads. */
static void cleanup_dead_threads() {
    thread_t *curr = dead_queue_head;
    dead_queue_head = NULL;

    while (curr) {
        thread_t *next = curr->next;
        thread_destroy(curr);
        curr = next;
    }
}

/**
 * @brief Initializes the scheduler subsystem and creates the BSP's main and idle threads.
 *
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t sched_init() {
    log_debug("initializing scheduler...");

    K_TRY(proc_init());

    // Allocate main BSP kernel thread
    virt_addr_t mem;
    K_TRY(kzalloc(sizeof(thread_t), &mem));
    thread_t *main_thread = (thread_t *)mem;

    process_t *kernel_process = proc_get_kernel_proc();

    main_thread->tid = 0;
    strncpy(main_thread->name, "kmain", sizeof(main_thread->name) - 1);
    main_thread->state = THREAD_RUNNING;
    main_thread->process = kernel_process;
    main_thread->time_slice = DEFAULT_TIME_SLICE;
    main_thread->cpu_affinity = 0; // kmain runs on Core 0
    fpu_state_init(main_thread->fpu_state);

    uint64_t current_rsp;
    ASM_CODE("mov %%rsp, %0" : "=r"(current_rsp));
    main_thread->kernel_stack_top = current_rsp;

    irq_flags_t pflags = spinlock_acquire_irqsave(&kernel_process->lock);
    main_thread->proc_next = kernel_process->threads;
    kernel_process->threads = main_thread;
    kernel_process->thread_count++;
    spinlock_release_irqrestore(&kernel_process->lock, pflags);

    cpu_local_t *bsp_cpu = smp_get_cpu_by_id(0);
    if (!bsp_cpu) {
        log_error("failed to get BSP CPU core 0");
        return E_NOTAVAIL;
    }

    bsp_cpu->current_thread = main_thread;

    thread_t *idle_thread = NULL;
    K_TRY(thread_create_on_cpu(kernel_process, idle_task, NULL, "idle_0", 0, &idle_thread));
    bsp_cpu->idle_thread = idle_thread;
    pop_next_ready_thread_for_cpu(0); // Remove idle task from ready queue

    log_debug("scheduler initialized for BSP");
    return E_OK;
}

/** @brief Enables preemptive scheduling globally. */
void sched_enable() { scheduler_enabled = true; }

/** @brief Performs preemptive context switching to the next ready thread on the calling CPU. */
void sched_schedule() {
    if (!scheduler_enabled)
        return;

    irq_flags_t flags = spinlock_acquire_irqsave(&sched_lock);

    cleanup_dead_threads();

    cpu_local_t *my_cpu = smp_get_current_cpu();
    if (!my_cpu) {
        log_error("scheduler enabled but no CPU found");
        spinlock_release_irqrestore(&sched_lock, flags);
        return;
    }

    thread_t *prev = my_cpu->current_thread;

    if (my_cpu->thread_to_enqueue != NULL) {
        scheduler_add_thread_locked(my_cpu->thread_to_enqueue);
        my_cpu->thread_to_enqueue = NULL;
    }

    thread_t *next = pop_next_ready_thread_for_cpu((int)my_cpu->cpu_id);
    if (!next) {
        if (prev && prev->state == THREAD_RUNNING) {
            prev->time_slice = DEFAULT_TIME_SLICE;
            spinlock_release_irqrestore(&sched_lock, flags);
            return;
        }
        next = my_cpu->idle_thread ? my_cpu->idle_thread : NULL;
    }

    if (prev && prev->state == THREAD_RUNNING) {
        if (strncmp(prev->name, "idle", 4) != 0 &&
            strcmp(prev->name, "kmain") != 0) {
            prev->state = THREAD_READY;
            my_cpu->thread_to_enqueue = prev;
        }
    } else if (prev && prev->state == THREAD_DEAD) {
        prev->next = dead_queue_head;
        dead_queue_head = prev;
    }

    if (next) {
        next->state = THREAD_RUNNING;
        next->time_slice = DEFAULT_TIME_SLICE;
        if (my_cpu) {
            my_cpu->current_thread = next;
            gdt_set_tss_kernel_stack(my_cpu->cpu_id, next->kernel_stack_top);
        }

        if (prev && prev->process != next->process && next->process) {
            ASM_CODE("mov %0, %%cr3" ::"r"(next->process->vmm_space->pml4_phys)
                     : "memory");
        }
    }

    if (prev != next && prev != NULL) {
        if ((uintptr_t)next->fpu_state % 16 != 0) {
            panic();
        }
        switch_context(&prev->rsp, next->rsp, prev->fpu_state, next->fpu_state);
    }

    spinlock_release_irqrestore(&sched_lock, flags);
}

/** @brief Voluntarily yields the remainder of the calling thread's time slice. */
void thread_yield() { sched_schedule(); }

/**
 * @brief Puts the current thread to sleep for the specified duration in milliseconds.
 *
 * @param[in] ms Duration in milliseconds to sleep.
 */
void thread_sleep_ms(uint64_t ms) {
    irq_flags_t flags = spinlock_acquire_irqsave(&sched_lock);

    cpu_local_t *my_cpu = smp_get_current_cpu();
    if (!my_cpu || !my_cpu->current_thread) {
        spinlock_release_irqrestore(&sched_lock, flags);
        return;
    }

    thread_t *thread = my_cpu->current_thread;

    thread->state = THREAD_SLEEPING;
    thread->sleep_until_tick = system_ticks + ms;

    thread->next = sleep_queue_head;
    thread->prev = NULL;
    if (sleep_queue_head) {
        sleep_queue_head->prev = thread;
    }
    sleep_queue_head = thread;

    spinlock_release_irqrestore(&sched_lock, flags);

    sched_schedule();
}

/** @brief Timer interrupt handler for the scheduler to update ticks and time slices. */
void sched_tick() {
    if (!scheduler_enabled)
        return;

    irq_flags_t flags = spinlock_acquire_irqsave(&sched_lock);

    cpu_local_t *my_cpu = smp_get_current_cpu();

    if (my_cpu && my_cpu->cpu_id == 0) {
        system_ticks++;

        thread_t *curr_sleep = sleep_queue_head;
        while (curr_sleep) {
            thread_t *next_sleep = curr_sleep->next;

            if (system_ticks >= curr_sleep->sleep_until_tick) {
                // Remove from sleep queue
                if (curr_sleep->prev) {
                    curr_sleep->prev->next = curr_sleep->next;
                } else {
                    sleep_queue_head = curr_sleep->next;
                }
                if (curr_sleep->next) {
                    curr_sleep->next->prev = curr_sleep->prev;
                }

                // Append to ready queue
                curr_sleep->state = THREAD_READY;
                curr_sleep->next = NULL;
                curr_sleep->prev = ready_queue_tail;
                if (ready_queue_tail) {
                    ready_queue_tail->next = curr_sleep;
                    ready_queue_tail = curr_sleep;
                } else {
                    ready_queue_head = curr_sleep;
                    ready_queue_tail = curr_sleep;
                }
            }
            curr_sleep = next_sleep;
        }
    }

    bool need_reschedule = false;
    if (my_cpu && my_cpu->current_thread &&
        my_cpu->current_thread->state == THREAD_RUNNING) {
        if (my_cpu->current_thread->time_slice > 0) {
            my_cpu->current_thread->time_slice--;
        }
        if (my_cpu->current_thread->time_slice == 0) {
            need_reschedule = true;
        }
    }

    spinlock_release_irqrestore(&sched_lock, flags);

    if (need_reschedule) {
        sched_schedule();
    }
}

/** @brief Terminates the execution of the calling thread and triggers rescheduling. */
void thread_exit() {
    irq_flags_t flags = spinlock_acquire_irqsave(&sched_lock);
    cpu_local_t *my_cpu = smp_get_current_cpu();
    if (my_cpu && my_cpu->current_thread) {
        my_cpu->current_thread->state = THREAD_DEAD;
    }
    spinlock_release_irqrestore(&sched_lock, flags);

    sched_schedule();
    while (1) {
        arch_hlt();
    }
}
