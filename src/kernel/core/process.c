/**
 * @file process.c
 * @brief Implementierung der Prozessverwaltung und -registrierung.
 * @details Bietet Funktionen zur Erstellung von Prozessen, Vergabe von eindeutigen
 *          Thread-sicheren PIDs und Anfügen von Prozessen an die globale Prozessliste.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <core/process.h>
#include <kernel/panic.h>
#include <lib/util/str.h>
#include <mm/memory.h>

process_t *kernel_process = NULL;
process_t *proc_list = NULL;

static uint64_t next_pid = 0;
static spinlock_t pid_lock = SPINLOCK_INIT;
static spinlock_t proc_list_lock = SPINLOCK_INIT;

void process_init() {
    kernel_process = process_create("kernel", vmm_kernel_space());
    if (!kernel_process) {
        panic();
    }

    serial_printf("Process: Initialized successfully (Kernel PID: %d)\n", kernel_process->pid);
}

process_t *process_create(const char *name, vmm_space_t *sp) {
    process_t *proc = (process_t *)kzalloc(sizeof(process_t));
    if (!proc) {
        return NULL;
    }

    // PID thread-sicher zuweisen
    uint64_t flags = spinlock_acquire_irqsave(&pid_lock);
    proc->pid = next_pid++;
    spinlock_release_irqrestore(&pid_lock, flags);

    // Name setzen
    if (name) {
        strncpy(proc->name, name, sizeof(proc->name) - 1);
    } else {
        strncpy(proc->name, "unnamed", sizeof(proc->name) - 1);
    }

    proc->vmm_space = sp;
    serial_printf("Process: pml4: %llx, cr3: %llx\n", (virt_addr_t)proc->vmm_space->pml4, (phys_addr_t)proc->vmm_space->pml4_phys);

    spinlock_init(&proc->lock);

    proc->threads = NULL;
    proc->thread_count = 0;

    process_register(proc);

    return proc;
}

void process_register(process_t *proc) {
    uint64_t flags = spinlock_acquire_irqsave(&proc_list_lock);

    proc->next = NULL;
    if (!proc_list) {
        proc_list = proc;
    } else {
        process_t *current = proc_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = proc;
    }

    spinlock_release_irqrestore(&proc_list_lock, flags);
}
