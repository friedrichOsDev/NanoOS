/**
 * @file sched.h
 * @brief Schnittstelle für den Round-Robin-Kernel-Scheduler.
 * @details Bietet Funktionen zur Verwaltung der Thread-Warteschlangen, Zeitsteuerung (Ticks),
 *          Zuweisung von CPU-Affinitäten und Kontextwechseln im Präemptiv-Betrieb.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>
#include <sched/thread.h>

void sched_init();
void sched_enable();
void sched_add_thread(thread_t *thread);
void sched_schedule();
void sched_tick();

void thread_set_affinity(thread_t *thread, int cpu_id);
void thread_sleep_ms(uint64_t ms);
void thread_yield();
void thread_exit();

uint64_t sched_get_ticks();
thread_t *sched_get_current_thread();
thread_t *pop_next_ready_thread_for_cpu(int cpu_id);

void idle_task(void *arg);
