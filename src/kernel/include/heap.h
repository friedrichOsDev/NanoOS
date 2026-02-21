/*
 * @file heap.h
 * @brief Header file for kernel heap
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <vmm.h>

#define HEAP_START VMM_HEAP_START
#define HEAP_ALIGNMENT 8
#define HEAP_PAGE_SIZE VMM_PAGE_SIZE
#define HEAP_INITIAL_PAGES 2
#define HEAP_INITIAL_SIZE (HEAP_INITIAL_PAGES * HEAP_PAGE_SIZE)
#define HEAP_MAX_SIZE (VMM_HEAP_END - VMM_HEAP_START)

typedef enum {
    HEAP_MAGIC_FREE = 0xDEADBEEF,
    HEAP_MAGIC_ALLOCATED = 0xBAADF00D
} heap_magic_t;

typedef struct heap_block {
    size_t size;
    heap_magic_t magic;
    struct heap_block* next;
} __attribute__((packed)) heap_block_t;

void heap_init(void);
virt_addr_t kmalloc(size_t size);
void kfree(virt_addr_t ptr);
virt_addr_t kzalloc(size_t size);
void kzfree(virt_addr_t ptr);
void heap_dump(void);
