/**
 * @file scheduler.c
 * @brief Preemptive Round-Robin Kernel Scheduler with Sleep Support
 * @author friedrichOsDev
 */

#include "core/thread.h"
#include <arch/x86_64/cpu/context.h>
#include <arch/x86_64/cpu/tss.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/panic.h>
#include <core/scheduler.h>
#include <lib/string.h>
#include <stdbool.h>

process_t *kernel_process = NULL;
thread_t *current_thread = NULL;

static thread_t *ready_queue_head = NULL;
static thread_t *ready_queue_tail = NULL;

static thread_t *sleep_queue_head = NULL;
static thread_t *dead_queue_head = NULL;

static spinlock_t sched_lock = SPINLOCK_INIT;
static thread_t *idle_thread = NULL;

static volatile uint64_t system_ticks = 0;
static bool scheduler_enabled = false;

static void idle_task(void *arg) {
    (void)arg;
    while (1) {
        __asm__ __volatile__("sti; hlt");
    }
}

uint64_t scheduler_get_ticks(void) {
    return system_ticks;
}

void scheduler_add_thread(thread_t *thread) {
    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

    thread->state = THREAD_READY;
    thread->next = NULL;
    thread->prev = ready_queue_tail;

    if (ready_queue_tail) {
        ready_queue_tail->next = thread;
        ready_queue_tail = thread;
    } else {
        ready_queue_head = thread;
        ready_queue_tail = thread;
    }

    spinlock_release_irqrestore(&sched_lock, flags);
}

static thread_t *pop_next_ready_thread(void) {
    if (!ready_queue_head)
        return NULL;

    thread_t *thread = ready_queue_head;
    ready_queue_head = thread->next;

    if (ready_queue_head) {
        ready_queue_head->prev = NULL;
    } else {
        ready_queue_tail = NULL;
    }

    thread->next = NULL;
    thread->prev = NULL;
    return thread;
}

void scheduler_init(void) {
    serial_printf(COM1, "SCHED: initializing scheduler...\n");

    // create kernel process
    kernel_process = (process_t *)kzalloc(sizeof(process_t));
    kernel_process->pid = 0;
    strncpy(kernel_process->name, "kernel", sizeof(kernel_process->name));
    kernel_process->pml4 = (page_table_t *)kernel_pml4;
    kernel_process->cr3 = V2P(kernel_pml4);

    // add current kernel_init_thread to kernel process
    thread_t *main_thread = (thread_t *)kzalloc(sizeof(thread_t));
    main_thread->tid = 0;
    strncpy(main_thread->name, "kmain", sizeof(main_thread->name));
    main_thread->state = THREAD_RUNNING;
    main_thread->process = kernel_process;
    main_thread->time_slice = DEFAULT_TIME_SLICE;

    uint64_t current_rsp;
    __asm__ __volatile__("mov %%rsp, %0" : "=r"(current_rsp));
    main_thread->kernel_stack_top = current_rsp;

    current_thread = main_thread;

    // create idle thread
    idle_thread = thread_create(kernel_process, idle_task, NULL, "idle");
    pop_next_ready_thread();

    scheduler_enabled = true;
    serial_printf(COM1, "SCHED: scheduler initialized. Main thread TID: 0\n");
}

static void cleanup_dead_threads(void) {
    thread_t *curr = dead_queue_head;
    dead_queue_head = NULL;

    while (curr) {
        thread_t *next = curr->next;
        if (curr->kernel_stack) {
            kfree((virt_addr_t)curr->kernel_stack);
        }
        kfree((virt_addr_t)curr);
        curr = next;
    }
}

void scheduler_schedule(void) {
    if (!scheduler_enabled) return;

    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

    cleanup_dead_threads();

    thread_t *prev = current_thread;
    thread_t *next = pop_next_ready_thread();

    if (!next) {
        if (prev->state == THREAD_RUNNING) {
            spinlock_release_irqrestore(&sched_lock, flags);
            return;
        }
        next = idle_thread;
    }

    if (prev->state == THREAD_RUNNING) {
        prev->state = THREAD_READY;
        if (ready_queue_tail) {
            ready_queue_tail->next = prev;
            prev->prev = ready_queue_tail;
            ready_queue_tail = prev;
        } else {
            ready_queue_head = prev;
            ready_queue_tail = prev;
        }
    } else if (prev->state == THREAD_DEAD) {
        prev->next = dead_queue_head;
        dead_queue_head = prev;
    }

    next->state = THREAD_RUNNING;
    next->time_slice = DEFAULT_TIME_SLICE;
    current_thread = next;

    // update TSS rsp0
    tss.rsp0 = next->kernel_stack_top;

    if (prev->process != next->process && next->process) {
        __asm__ __volatile__("mov %0, %%cr3" ::"r"(next->process->cr3) : "memory");
    }

    spinlock_release_irqrestore(&sched_lock, flags);

    if (prev != next) {
        switch_context(&prev->rsp, next->rsp);
    }
}

void thread_yield(void) { scheduler_schedule(); }

void thread_sleep_ms(uint64_t ms) {
    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

    current_thread->state = THREAD_SLEEPING;
    current_thread->sleep_until_tick = system_ticks + ms;

    // add to sleep queue
    current_thread->next = sleep_queue_head;
    current_thread->prev = NULL;
    if (sleep_queue_head) {
        sleep_queue_head->prev = current_thread;
    }
    sleep_queue_head = current_thread;

    spinlock_release_irqrestore(&sched_lock, flags);

    scheduler_schedule();
}

void scheduler_thread_exit(void) {
    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);
    current_thread->state = THREAD_DEAD;
    spinlock_release_irqrestore(&sched_lock, flags);

    scheduler_schedule();
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

void scheduler_tick(void) {
    if (!scheduler_enabled) return;

    system_ticks++;

    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

    thread_t *curr_sleep = sleep_queue_head;
    while (curr_sleep) {
        thread_t *next_sleep = curr_sleep->next;

        if (system_ticks >= curr_sleep->sleep_until_tick) {
            // remove from sleep queue
            if (curr_sleep->prev) {
                curr_sleep->prev->next = curr_sleep->next;
            } else {
                sleep_queue_head = curr_sleep->next;
            }
            if (curr_sleep->next) {
                curr_sleep->next->prev = curr_sleep->prev;
            }

            // add to ready queue
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

    bool need_reschedule = false;
    if (current_thread && current_thread->state == THREAD_RUNNING) {
        if (current_thread->time_slice > 0) {
            current_thread->time_slice--;
        }
        if (current_thread->time_slice == 0) {
            need_reschedule = true;
        }
    }

    spinlock_release_irqrestore(&sched_lock, flags);

    if (need_reschedule) {
        scheduler_schedule();
    }
}