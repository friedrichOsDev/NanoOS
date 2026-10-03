/**
 * @file context.h
 * @brief Deklarationen für den Kontextwechsel in Assembler.
 * @author friedrichOsDev
 */

#pragma once

#include <core/thread.h>

/**
 * @brief Führt einen Kontextwechsel zwischen zwei Threads durch.
 *
 * Speichert den aktuellen Registerzustand (callee-saved Register RBP, RBX, R12–R15)
 * sowie optional den FPU/SSE-Zustand des vorherigen Threads auf dessen Stack.
 * Wechselt anschließend auf den Stack des nächsten Threads und stellt dessen
 * Register- und FPU/SSE-Zustand wieder her.
 *
 * @param prev_rsp_ptr Zeiger auf das RSP-Feld des vorherigen Threads (&prev->rsp),
 *                     in dem der aktuelle Stack-Pointer gespeichert wird.
 * @param next_rsp     Der gespeicherte Stack-Pointer (next->rsp) des Ziel-Threads.
 * @param prev_fpu_state Zeiger auf den Speicherbereich für den FPU/SSE-Zustand des
 *                       vorherigen Threads (kann NULL sein).
 * @param next_fpu_state Zeiger auf den Speicherbereich für den FPU/SSE-Zustand des
 *                       nächsten Threads (kann NULL sein).
 */
extern void switch_context(uint64_t *prev_rsp_ptr, uint64_t next_rsp, void *prev_fpu_state, void *next_fpu_state);

/**
 * @brief Einstiegspunkt (Stub) für neu erstellte Threads.
 *
 * Wird beim ersten Kontextwechsel in einen neu erstellten Thread angesprungen.
 * Gibt den anfänglichen Scheduler-Lock frei, aktiviert die Interrupts (`sti`),
 * ruft die Thread-Funktion mit dem vorgegebenen Argument auf und beendet den Thread
 * anschließend ordnungsgemäß über `thread_exit()`.
 *
 * @note Erwartet die Thread-Funktion in Register R12 und das Argument (void * arg) in Register R13.
 */
extern void thread_entry_stub();