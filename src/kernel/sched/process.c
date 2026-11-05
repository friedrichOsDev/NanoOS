/**
 * @file process.c
 * @brief Process initialization and creation.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <kernel/panic.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <sched/process.h>

static process_t *kernel_process = NULL;
static process_t *proc_list = NULL;
static uint64_t next_pid = 0;
static spinlock_t pid_lock = SPINLOCK_INIT;
static spinlock_t proc_list_lock = SPINLOCK_INIT;

/**
 * @brief Initializes the process subsystem and creates the initial kernel process.
 *
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t proc_init() {
    K_TRY(proc_create("kernel", vmm_kernel_space(), &kernel_process));
    log_debug("kernel process initialized successfully (Kernel PID: %d)", kernel_process->pid);
    return E_OK;
}

/**
 * @brief Creates a new process with the specified name and virtual address space.
 *
 * @param[in] name Name of the process.
 * @param[in] sp Pointer to the virtual memory space for the process.
 * @param[out] out Pointer to store the created process structure.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t proc_create(const char *name, vmm_space_t *sp, process_t **out) {
    virt_addr_t mem;
    K_TRY(kzalloc(sizeof(process_t), &mem));
    process_t *proc = (process_t *)mem;

    // PID thread-sicher zuweisen
    irq_flags_t flags = spinlock_acquire_irqsave(&pid_lock);
    proc->pid = next_pid++;
    spinlock_release_irqrestore(&pid_lock, flags);

    if (name) {
        strncpy(proc->name, name, sizeof(proc->name) - 1);
    } else {
        strncpy(proc->name, "unnamed", sizeof(proc->name) - 1);
    }

    proc->vmm_space = sp;
    log_trace("create process with name: %s, pid: %d, pml4: %llx, cr3: %llx", proc->name, proc->pid, (virt_addr_t)proc->vmm_space->pml4, (phys_addr_t)proc->vmm_space->pml4_phys);

    spinlock_init(&proc->lock);

    proc->threads = NULL;
    proc->thread_count = 0;

    proc_register(proc);

    *out = proc;
    return E_OK;
}

/**
 * @brief Destroys a process, cleans up its virtual address space (if not kernel), and frees its memory.
 *
 * @param[in,out] proc Pointer to the process to destroy.
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t proc_destroy(process_t *proc) {
    if (!proc)
        return E_INVAL;

    if (proc == kernel_process) {
        log_error("attempted to destroy the main kernel process");
        return E_INVAL;
    }

    proc_unregister(proc);

    if (proc->vmm_space && proc->vmm_space != vmm_kernel_space()) {
        K_TRY(vmm_space_destroy(proc->vmm_space));
    }

    log_trace("destroy process with name: %s, pid: %d", proc->name, proc->pid);

    return kfree((virt_addr_t)proc);
}

/**
 * @brief Registers a process in the global process list thread-safely.
 *
 * @param[in,out] proc Pointer to the process to register.
 */
void proc_register(process_t *proc) {
    irq_flags_t flags = spinlock_acquire_irqsave(&proc_list_lock);

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

/**
 * @brief Unregisters a process from the global process list thread-safely.
 *
 * @param[in,out] proc Pointer to the process to unregister.
 */
void proc_unregister(process_t *proc) {
    if (!proc)
        return;

    irq_flags_t flags = spinlock_acquire_irqsave(&proc_list_lock);

    if (proc_list == proc) {
        proc_list = proc->next;
    } else {
        process_t *current = proc_list;
        while (current != NULL && current->next != proc) {
            current = current->next;
        }
        if (current != NULL) {
            current->next = proc->next;
        }
    }

    spinlock_release_irqrestore(&proc_list_lock, flags);
}

/**
 * @brief Retrieves the main kernel process structure.
 *
 * @return Pointer to the kernel process.
 */
process_t *proc_get_kernel_proc() {
    return kernel_process;
}

/**
 * @brief Retrieves the head of the global process list.
 *
 * @return Pointer to the first process in the list.
 */
process_t *proc_get_list() {
    return proc_list;
}

/**
 * @brief Retrieves the spinlock protecting the global process list.
 *
 * @return Pointer to the process list spinlock.
 */
spinlock_t *proc_get_list_lock() {
    return &proc_list_lock;
}
