/**
 * @file heap.h
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

/**
 * @brief Magic numbers to identify the status of a heap block.
 */
typedef enum {
    HEAP_MAGIC_FREE = 0xDEADBEEF,      /**< Block is free and available for allocation. */
    HEAP_MAGIC_ALLOCATED = 0xBAADF00D  /**< Block is currently allocated. */
} heap_magic_t;

/**
 * @brief Header for each memory block in the heap.
 */
typedef struct heap_block {
    size_t size;               /**< Size of the data area in bytes. */
    heap_magic_t magic;        /**< Magic number indicating block status. */
    struct heap_block* next;   /**< Pointer to the next block in the list. */
} __attribute__((packed)) heap_block_t;

void heap_init(void);
virt_addr_t kmalloc(size_t size);
void kfree(virt_addr_t ptr);
virt_addr_t kzalloc(size_t size);
void kzfree(virt_addr_t ptr);
void heap_dump(void);
heap_block_t* heap_get_list(void);
