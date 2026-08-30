/**
 * @file scheduler.h
 * @brief Round-Robin Kernel Scheduler
 * @author friedrichOsDev
 */

#pragma once

#include <core/thread.h>
#include <stdint.h>

void scheduler_init(void);
void scheduler_enable();
void scheduler_add_thread(thread_t *thread);
void scheduler_schedule(void);
void thread_yield(void);
void thread_sleep_ms(uint64_t ms);
void scheduler_thread_exit(void);
void scheduler_tick(void);
uint64_t scheduler_get_ticks(void);

thread_t *scheduler_get_current_thread(void);
thread_t *pop_next_ready_thread_for_cpu(int cpu_id);
void idle_task(void *arg);
void thread_set_affinity(thread_t *thread, int cpu_id);