/**
 * @file scheduler.h
 * @brief Schnittstelle für den Round-Robin-Kernel-Scheduler.
 * @details Bietet Funktionen zur Verwaltung der Thread-Warteschlangen, Zeitsteuerung (Ticks),
 *          Zuweisung von CPU-Affinitäten und Kontextwechseln im Präemptiv-Betrieb.
 * @author friedrichOsDev
 */

#pragma once

#include <core/thread.h>
#include <stdint.h>

/**
 * @brief Initialisiert das Scheduler-Subsystem für den Bootstrap Processor (BSP).
 */
void scheduler_init();

/**
 * @brief Aktiviert den Preemptive Scheduler.
 */
void scheduler_enable();

/**
 * @brief Fügt einen Thread zur Bereit-Warteschlange (Ready Queue) hinzu.
 * @param thread Zeiger auf den hinzuzufügenden Thread.
 */
void scheduler_add_thread(thread_t *thread);

/**
 * @brief Führt den Kernelscheduler aus und wechselt bei Bedarf den Kontext.
 */
void scheduler_schedule();

/**
 * @brief Gibt die verbleibende Rechenzeit freiwillig ab (Yield).
 */
void thread_yield();

/**
 * @brief Versetzt den aktuellen Thread für eine bestimmte Zeitdauer in den Schlafzustand.
 * @param ms Zeitdauer in Millisekunden.
 */
void thread_sleep_ms(uint64_t ms);

/**
 * @brief Beendet den aktuellen Thread und leitet die Bereinigung ein.
 */
void scheduler_thread_exit();

/**
 * @brief Verarbeitet einen System-Timer-Tick (Inkrementiert Ticks, prüft Schlafzustände und Zeitscheiben).
 */
void scheduler_tick();

/**
 * @brief Liefert die bisher vernebelten System-Ticks seit dem Systemstart.
 * @return Ticks als 64-Bit-Ganzzahl.
 */
uint64_t scheduler_get_ticks();

/**
 * @brief Gibt den aktuell auf der aufrufenden CPU laufenden Thread zurück.
 * @return Zeiger auf den aktuellen Thread oder NULL, falls nicht ermittelbar.
 */
thread_t *scheduler_get_current_thread();

/**
 * @brief Entnimmt der Bereit-Warteschlange den nächsten lauffähigen Thread für eine bestimmte CPU.
 * @param cpu_id ID der Ziel-CPU.
 * @return Zeiger auf den bereiten Thread oder NULL, falls kein passender Thread vorhanden ist.
 */
thread_t *pop_next_ready_thread_for_cpu(int cpu_id);

/**
 * @brief Idle-Task-Routine, die ausgeführt wird, wenn keine sonstigen Threads lauffähig sind.
 * @param arg Nicht genutzter Zeiger auf Argumente.
 */
void idle_task(void *arg);

/**
 * @brief Legt fest, auf welcher CPU ein bestimmter Thread ausgeführt werden darf.
 * @param thread Zeiger auf den Ziel-Thread.
 * @param cpu_id ID der Ziel-CPU (-1 für beliebige CPU).
 */
void thread_set_affinity(thread_t *thread, int cpu_id);