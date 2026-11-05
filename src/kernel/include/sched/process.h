/**
 * @file process.h
 * @brief Process definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <kernel/sync.h>
#include <mm/memory.h>
#include <pch.h>

struct thread;

/** @brief Represents a process in the system. */
typedef struct process {
    uint64_t pid;           /**< Unique Process ID (PID) */
    char name[32];          /**< Name of the process */
    vmm_space_t *vmm_space; /**< Pointer to the process address space (including pml4, pml4_phys, lock) */
    struct thread *threads; /**< Pointer to the list of associated threads */
    size_t thread_count;    /**< Number of threads in the process */
    spinlock_t lock;        /**< Spinlock protecting the process data structure */
    struct process *next;   /**< Pointer to the next process in the global process list */
} process_t;

kerr_t proc_init();
kerr_t proc_create(const char *name, vmm_space_t *sp, process_t **out);
kerr_t proc_destroy(process_t *proc);
void proc_register(process_t *proc);
void proc_unregister(process_t *proc);
process_t *proc_get_kernel_proc();
process_t *proc_get_list();
spinlock_t *proc_get_list_lock();
