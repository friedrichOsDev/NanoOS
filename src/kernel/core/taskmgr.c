/**
 * @file taskmgr.c
 * @brief Serieller Task-Manager und Prozessauflistung.
 * @author friedrichOsDev
 */

#include <arch/arch.h>

#include <core/taskmgr.h>
#include <lib/print.h>
#include <sched/process.h>
#include <sched/thread.h>

/**
 * @brief Spinlock zur Absicherung des Zugriffs auf die globale Prozessliste während der Ausgabe.
 */
static spinlock_t proc_list_lock = SPINLOCK_INIT;

/**
 * @brief Wandelt einen Thread-Zustand (`thread_state_t`) in eine lesbare Zeichenkette um.
 * @param state Der abzufragende Thread-Zustand.
 * @return Eine statische Zeichenkette, die den Namen des Zustands repräsentiert (z. B. "READY", "RUNNING").
 */
static const char *thread_state_to_string(thread_state_t state) {
    switch (state) {
    case THREAD_EMBRYO:
        return "EMBRYO";
    case THREAD_READY:
        return "READY";
    case THREAD_RUNNING:
        return "RUNNING";
    case THREAD_BLOCKED:
        return "BLOCKED";
    case THREAD_SLEEPING:
        return "SLEEPING";
    case THREAD_DEAD:
        return "DEAD";
    default:
        return "UNKNOWN";
    }
}

void ps_dump() {
    irq_flags_t list_flags = spinlock_acquire_irqsave(&proc_list_lock);

    serial_printf("\n============================= TASK MANAGER ==============================\n");
    serial_printf("%-6s %-20s %-6s %-20s %-10s %-6s\n", "PID", "PROCESS", "TID", "THREAD NAME", "STATE", "CORE");
    serial_printf("-------------------------------------------------------------------------\n");

    for (process_t *p = proc_get_list(); p != NULL; p = p->next) {
        irq_flags_t pflags = spinlock_acquire_irqsave(&p->lock);

        for (thread_t *t = p->threads; t != NULL; t = t->proc_next) {
            char core_str[8];
            if (t->cpu_affinity < 0) {
                snprintf(core_str, sizeof(core_str), "ANY");
            } else {
                snprintf(core_str, sizeof(core_str), "%d", t->cpu_affinity);
            }

            serial_printf("%-6d %-20s %-6d %-20s %-10s %-6s\n", p->pid, p->name, t->tid, t->name, thread_state_to_string(t->state), core_str);
        }

        spinlock_release_irqrestore(&p->lock, pflags);
    }

    serial_printf("-------------------------------------------------------------------------\n");
    serial_printf("%-11s %-15s %-27s %-17s\n", "CPU CORE", "ACTIVE TID", "ACTIVE THREAD", "STATUS");
    serial_printf("-------------------------------------------------------------------------\n");

    for (uint32_t i = 0; i < smp_get_cpu_count(); i++) {
        if (smp_get_cpu_by_id(i)->online) {
            thread_t *curr_t = smp_get_cpu_by_id(i)->current_thread;
            if (curr_t) {
                serial_printf("%-11d %-15d %-27s %-17s\n", smp_get_cpu_by_id(i)->cpu_id, curr_t->tid, curr_t->name, "ONLINE");
            } else {
                serial_printf("%-11d %-15s %-27s %-17s\n", smp_get_cpu_by_id(i)->cpu_id, "-", "none", "ONLINE");
            }
        } else {
            serial_printf("%-11d %-15s %-27s %-17s\n", smp_get_cpu_by_id(i)->cpu_id, "-", "-", "OFFLINE");
        }
    }

    serial_printf("=========================================================================\n\n");

    spinlock_release_irqrestore(&proc_list_lock, list_flags);
}
