/**
 * @file scheduler.h
 * @brief Round-Robin Kernel Scheduler
 * @author friedrichOsDev
 */

#pragma once

#include <core/thread.h>

void scheduler_init(void);
void scheduler_add_thread(thread_t *thread);
void scheduler_schedule(void);
void thread_yield(void);
void scheduler_thread_exit(void);
void scheduler_tick(void);

extern thread_t *current_thread;