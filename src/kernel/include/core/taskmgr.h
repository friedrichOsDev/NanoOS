/**
 * @file taskmgr.h
 * @brief Serieller Task-Manager und die Prozessauflistung.
 * @author friedrichOsDev
 */

#pragma once

#include <core/process.h>

/**
 * @brief Gibt eine strukturierte Übersicht der laufenden Prozesse, Threads und SMP-CPU-Zustände über COM1 aus.
 * @details Akquiriert die erforderlichen Spinlocks, um die Prozessliste sowie die Threads jedes Prozesses
 * sicher zu durchlaufen. Gibt PID, Prozessname, TID, Threadname, Status und CPU-Affinität aus.
 * Anschließend wird der Status aller bekannten CPU-Kerne (Online/Offline, aktiver Thread) aufgelistet.
 *
 * @param proc_list Zeiger auf den Kopf der globalen Prozessliste (`process_t`).
 */
void ps_dump(process_t *proc_list);
