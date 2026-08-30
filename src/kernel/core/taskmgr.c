/**
 * @file taskmgr.c
 * @brief Simple Serial Task Manager / Process Listing
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <core/taskmgr.h>
#include <core/thread.h>
#include <lib/print.h>

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

void ps_dump(process_t *proc_list) {
    serial_printf(COM1, "\n========================= TASK MANAGER "
                        "==========================\n");
    serial_printf(COM1, "%-6s %-16s %-6s %-16s %-10s %-6s\n", "PID", "PROCESS",
                  "TID", "THREAD NAME", "STATE", "CORE");
    serial_printf(
        COM1,
        "-----------------------------------------------------------------\n");

    for (process_t *p = proc_list; p != NULL; p = p->next) {
        uint64_t pflags = spinlock_acquire_irqsave(&p->lock);

        // Hier proc_next nutzen!
        for (thread_t *t = p->threads; t != NULL; t = t->proc_next) {
            char core_str[8];
            if (t->cpu_affinity < 0) {
                snprintf(core_str, sizeof(core_str), "ANY");
            } else {
                snprintf(core_str, sizeof(core_str), "%d", t->cpu_affinity);
            }

            serial_printf(COM1, "%-6d %-16s %-6d %-16s %-10s %-6s\n", p->pid,
                          p->name, t->tid, t->name,
                          thread_state_to_string(t->state), core_str);
        }

        spinlock_release_irqrestore(&p->lock, pflags);
    }

    serial_printf(COM1, "======================================================"
                        "===========\n\n");
}