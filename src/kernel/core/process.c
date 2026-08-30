/**
 * @file process.c
 * @brief Process implementation
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/process.h>
#include <lib/string.h>

process_t *kernel_process = NULL;
process_t *proc_list = NULL;

static uint64_t next_pid = 0;
static spinlock_t pid_lock = SPINLOCK_INIT;
static spinlock_t proc_list_lock = SPINLOCK_INIT;

void process_init(void) {
    kernel_process = process_create("kernel", (page_table_t *)V2P(kernel_pml4));
    if (!kernel_process) {
        serial_printf(COM1, "Process: Failed to initialize kernel process!\n");
        return;
    }

    serial_printf(COM1, "Process: Initialized successfully (Kernel PID: %d)\n",
                  kernel_process->pid);
}

process_t *process_create(const char *name, page_table_t *pml4) {
    process_t *proc = (process_t *)kzalloc(sizeof(process_t));
    if (!proc) {
        return NULL;
    }

    // set PID thread-safe
    uint64_t flags = spinlock_acquire_irqsave(&pid_lock);
    proc->pid = next_pid++;
    spinlock_release_irqrestore(&pid_lock, flags);

    // set Name
    if (name) {
        strncpy(proc->name, name, sizeof(proc->name) - 1);
    } else {
        strncpy(proc->name, "unnamed", sizeof(proc->name) - 1);
    }

    proc->pml4 = pml4;
    proc->cr3 = V2P((virt_addr_t)pml4);

    spinlock_init(&proc->lock);

    proc->threads = NULL;
    proc->thread_count = 0;

    uint64_t list_flags = spinlock_acquire_irqsave(&proc_list_lock);
    proc->next = proc_list;
    proc_list = proc;
    spinlock_release_irqrestore(&proc_list_lock, list_flags);

    return proc;
}