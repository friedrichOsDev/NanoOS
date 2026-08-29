/**
 * @file scheduler.c
 * @brief Scheduler implementation
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/context.h>
#include <arch/x86_64/cpu/tss.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/panic.h>
#include <core/scheduler.h>
#include <lib/string.h>

process_t *kernel_process = NULL;
thread_t *current_thread = NULL;

static thread_t *ready_queue_head = NULL;
static thread_t *ready_queue_tail = NULL;
static spinlock_t sched_lock = SPINLOCK_INIT;

static thread_t *idle_thread = NULL;

static void idle_task(void *arg) {
    (void)arg;
    while (1) {
        __asm__ __volatile__("sti; hlt");
    }
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

    current_thread = main_thread;

    // create idle thread
    idle_thread = thread_create(kernel_process, idle_task, NULL, "idle");
    pop_next_ready_thread();

    serial_printf(COM1, "SCHED: scheduler initialized. Main thread TID: 0\n");
}

void scheduler_schedule(void) {
    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

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
    }

    next->state = THREAD_RUNNING;
    next->time_slice = DEFAULT_TIME_SLICE;
    current_thread = next;

    tss.rsp0 = next->kernel_stack_top;

    if (prev->process != next->process && next->process) {
        __asm__ __volatile__("mov %0, %%cr3" ::"r"(next->process->cr3)
                             : "memory");
    }

    spinlock_release_irqrestore(&sched_lock, flags);

    if (prev != next) {
        switch_context(&prev->rsp, next->rsp);
    }
}

void thread_yield(void) { scheduler_schedule(); }

void scheduler_thread_exit(void) {
    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);
    current_thread->state = THREAD_DEAD;
    spinlock_release_irqrestore(&sched_lock, flags);

    scheduler_schedule();
    while (1) {
        __asm__ __volatile__("hlt");
    }
}