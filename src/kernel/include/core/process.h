/**
 * @file process.h
 * @brief Process definitions
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <core/sync.h>
#include <stddef.h>
#include <stdint.h>

struct thread;

typedef struct process {
    uint64_t pid;
    char name[32];

    page_table_t *pml4;
    phys_addr_t cr3;

    struct thread *threads;
    size_t thread_count;

    spinlock_t lock;
    struct process *next;
} process_t;

extern process_t *kernel_process;

void process_init(void);
process_t *process_create(const char *name, page_table_t *pml4);