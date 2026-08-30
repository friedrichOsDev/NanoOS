/**
 * @file scheduler.c
 * @brief Preemptive Round-Robin Kernel Scheduler with Sleep Support
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/context.h>
#include <arch/x86_64/cpu/smp.h>
#include <arch/x86_64/cpu/tss.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/panic.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <lib/string.h>
#include <stdbool.h>

static thread_t *ready_queue_head = NULL;
static thread_t *ready_queue_tail = NULL;
static thread_t *sleep_queue_head = NULL;
static thread_t *dead_queue_head = NULL;

static spinlock_t sched_lock = SPINLOCK_INIT;
static volatile uint64_t system_ticks = 0;
static bool scheduler_enabled = false;

void idle_task(void *arg) {
    (void)arg;
    while (1) {
        __asm__ __volatile__("sti; hlt");
    }
}

thread_t *scheduler_get_current_thread(void) {
    cpu_local_t *cpu = smp_get_current_cpu();
    return cpu ? cpu->current_thread : NULL;
}

uint64_t scheduler_get_ticks(void) { return system_ticks; }

void thread_set_affinity(thread_t *thread, int cpu_id) {
    if (thread) {
        uint64_t flags = spinlock_acquire_irqsave(&sched_lock);
        thread->cpu_affinity = cpu_id;
        spinlock_release_irqrestore(&sched_lock, flags);
    }
}

void scheduler_add_thread(thread_t *thread) {
    if (strncmp(thread->name, "idle", 4) == 0) {
        return;
    }

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

    if (smp_cpu_count > 1) {
        lapic_send_broadcast_reschedule_ipi();
    }
}

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

static void cleanup_dead_threads(void) {
    thread_t *curr = dead_queue_head;
    dead_queue_head = NULL;

    while (curr) {
        thread_t *next = curr->next;

        if (curr->process) {
            uint64_t pflags = spinlock_acquire_irqsave(&curr->process->lock);
            thread_t **pp = &curr->process->threads;
            while (*pp) {
                if (*pp == curr) {
                    *pp = curr->proc_next;
                    curr->process->thread_count--;
                    break;
                }
                pp = &(*pp)->proc_next;
            }
            spinlock_release_irqrestore(&curr->process->lock, pflags);
        }

        if (curr->kernel_stack) {
            kfree((virt_addr_t)curr->kernel_stack);
        }

        if (strcmp(curr->name, "kmain") != 0) {
            kfree((virt_addr_t)curr);
        }

        curr = next;
    }
}

void scheduler_init(void) {
    serial_printf(COM1, "SCHED: initializing scheduler...\n");

    process_init();

    // create kernel thread
    thread_t *main_thread = (thread_t *)kzalloc(sizeof(thread_t));
    if (!main_thread) {
        panic("SCHED: Failed to allocate main thread structure", 0);
    }

    main_thread->tid = 0;
    strncpy(main_thread->name, "kmain", sizeof(main_thread->name) - 1);
    main_thread->state = THREAD_RUNNING;
    main_thread->process = kernel_process;
    main_thread->time_slice = DEFAULT_TIME_SLICE;
    main_thread->cpu_affinity = 0; // kmain läuft auf Core 0

    uint64_t current_rsp;
    __asm__ __volatile__("mov %%rsp, %0" : "=r"(current_rsp));
    main_thread->kernel_stack_top = current_rsp;

    uint64_t pflags = spinlock_acquire_irqsave(&kernel_process->lock);
    main_thread->proc_next = kernel_process->threads;
    kernel_process->threads = main_thread;
    kernel_process->thread_count++;
    spinlock_release_irqrestore(&kernel_process->lock, pflags);

    cpus[0].current_thread = main_thread;
    cpus[0].idle_thread =
        thread_create_on_cpu(kernel_process, idle_task, NULL, "idle_0", 0);
    pop_next_ready_thread_for_cpu(0); // remove idle task from queue

    serial_printf(COM1, "SCHED: scheduler initialized for BSP.\n");
}

void scheduler_enable() { scheduler_enabled = true; }

void scheduler_schedule(void) {
    if (!scheduler_enabled)
        return;

    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

    cleanup_dead_threads();

    cpu_local_t *my_cpu = smp_get_current_cpu();
    thread_t *prev = my_cpu->current_thread;
    thread_t *next = pop_next_ready_thread_for_cpu(my_cpu->cpu_id);

    if (!next) {
        if (prev && prev->state == THREAD_RUNNING) {
            prev->time_slice = DEFAULT_TIME_SLICE;
            spinlock_release_irqrestore(&sched_lock, flags);
            return;
        }
        next = my_cpu ? my_cpu->idle_thread : NULL;
    }

    if (prev && prev->state == THREAD_RUNNING) {
        if (strncmp(prev->name, "idle", 4) != 0 &&
            strcmp(prev->name, "kmain") != 0) {
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
    } else if (prev && prev->state == THREAD_DEAD) {
        prev->next = dead_queue_head;
        dead_queue_head = prev;
    }

    if (next) {
        next->state = THREAD_RUNNING;
        next->time_slice = DEFAULT_TIME_SLICE;
        if (my_cpu) {
            my_cpu->current_thread = next;
        }
        tss.rsp0 = next->kernel_stack_top;

        if (prev && prev->process != next->process && next->process) {
            __asm__ __volatile__("mov %0, %%cr3" ::"r"(next->process->cr3)
                                 : "memory");
        }
    }

    spinlock_release_irqrestore(&sched_lock, flags);

    if (prev != next && prev != NULL) {
        switch_context(&prev->rsp, next->rsp);
    }
}

void thread_yield(void) { scheduler_schedule(); }

void thread_sleep_ms(uint64_t ms) {
    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

    cpu_local_t *my_cpu = smp_get_current_cpu();
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

    scheduler_schedule();
}

void scheduler_tick(void) {
    if (!scheduler_enabled)
        return;

    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);

    cpu_local_t *my_cpu = smp_get_current_cpu();

    if (my_cpu && my_cpu->cpu_id == 0) {
        system_ticks++;

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
        scheduler_schedule();
    }
}

void scheduler_thread_exit(void) {
    uint64_t flags = spinlock_acquire_irqsave(&sched_lock);
    cpu_local_t *my_cpu = smp_get_current_cpu();
    my_cpu->current_thread->state = THREAD_DEAD;
    spinlock_release_irqrestore(&sched_lock, flags);

    scheduler_schedule();
    while (1) {
        __asm__ __volatile__("hlt");
    }
}