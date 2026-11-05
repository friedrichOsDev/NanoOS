/**
 * @file heap.c
 * @brief Kernel HEAP allocator implementation.
 * @author friedrichOsDev
 */

#include "mem_def.h"
#include <arch/arch.h>
#include <kernel/panic.h>
#include <kernel/sync.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <sched/taskmgr.h>

static heap_node_t *heap_list_head = NULL;
static virt_addr_t heap_end_addr = 0;
static spinlock_t heap_lock = SPINLOCK_INIT;

/**
 * @brief Initializes the dynamic kernel heap allocator, allocating initial physical pages and setting up the head block.
 *
 * @return Returns E_OK on success, or an error code on failure.
 */
kerr_t heap_init() {
    phys_addr_t page1;
    phys_addr_t page2;
    phys_addr_t page3;
    phys_addr_t page4;
    K_TRY(pmm_page_alloc(&page1));
    K_TRY(pmm_page_alloc(&page2));
    K_TRY(pmm_page_alloc(&page3));
    K_TRY(pmm_page_alloc(&page4));
    K_TRY(vmm_map_page(vmm_kernel_space(), KERNEL_HEAP_START, page1, PTE_WRITABLE));
    K_TRY(vmm_map_page(vmm_kernel_space(), KERNEL_HEAP_START + PAGE_SIZE, page2, PTE_WRITABLE));
    K_TRY(vmm_map_page(vmm_kernel_space(), KERNEL_HEAP_START + PAGE_SIZE * 2, page3, PTE_WRITABLE));
    K_TRY(vmm_map_page(vmm_kernel_space(), KERNEL_HEAP_START + PAGE_SIZE * 3, page4, PTE_WRITABLE));

    memset((void *)KERNEL_HEAP_START, 0, PAGE_SIZE * 4);

    size_t initial_size = PAGE_SIZE * 4;
    heap_node_t *initial_block = (heap_node_t *)KERNEL_HEAP_START;
    initial_block->magic = HEAP_MAGIC_FREE;
    initial_block->size = initial_size;
    initial_block->payload_size = initial_size - HEAP_HEADER_SIZE;
    initial_block->prev = NULL;
    initial_block->next = NULL;

    heap_list_head = initial_block;
    heap_end_addr = KERNEL_HEAP_START + initial_size;

    log_debug("initial_heap_block at %llx with size %llx", initial_block, initial_size);
    return E_OK;
}

/**
 * @brief Extends the kernel heap by allocating additional physical pages and appending or coalescing a new block.
 *
 * @param[in] size Minimum required size in bytes to extend.
 * @param[out] out Pointer to store the resulting heap node block pointer.
 * @return Returns E_OK on success, or an error code on failure.
 */
static kerr_t heap_extend(size_t size, heap_node_t **out) {
    size_t needed_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    virt_addr_t extension_start = heap_end_addr;

    for (size_t i = 0; i < needed_pages; i++) {
        phys_addr_t phys_page;
        K_TRY(pmm_page_alloc(&phys_page));
        K_TRY(vmm_map_page(vmm_kernel_space(), extension_start + (i * PAGE_SIZE), phys_page, PTE_WRITABLE));
    }

    size_t extended_bytes = needed_pages * PAGE_SIZE;

    memset((void *)extension_start, 0, extended_bytes);

    heap_node_t *new_block = (heap_node_t *)extension_start;
    new_block->magic = HEAP_MAGIC_FREE;
    new_block->size = extended_bytes;
    new_block->payload_size = extended_bytes - HEAP_HEADER_SIZE;
    new_block->prev = NULL;
    new_block->next = NULL;

    heap_end_addr += extended_bytes;

    heap_node_t *current = heap_list_head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = new_block;
    new_block->prev = current;

    if (current->magic == HEAP_MAGIC_FREE) {
        current->size += new_block->size;
        current->payload_size = current->size - HEAP_HEADER_SIZE;
        current->next = NULL;
        *out = current;
        return E_OK;
    }

    *out = new_block;
    return E_OK;
}

/**
 * @brief Allocates a block of dynamic kernel memory using a best-fit algorithm.
 *
 * @param[in] size Requested size in bytes.
 * @param[out] out Pointer to store the virtual address of the allocated payload.
 * @return Returns E_OK on success, or an error code (E_BADSIZE, E_NOMEM) on failure.
 */
kerr_t kmalloc(size_t size, virt_addr_t *out) {
    if (size == 0)
        return E_BADSIZE;

    irq_flags_t flags = spinlock_acquire_irqsave(&heap_lock);

    size = (size + 15U) & ~15U;
    size_t total_required_size = size + HEAP_HEADER_SIZE;

    heap_node_t *current = heap_list_head;
    heap_node_t *best_fit = NULL;
    size_t min_suitable_size = (size_t)-1; // Max size_t Value

    // Best-Fit
    while (current != NULL) {
        if (current->magic == HEAP_MAGIC_FREE &&
            current->size >= total_required_size) {
            if (current->size < min_suitable_size) {
                best_fit = current;
                min_suitable_size = current->size;

                if (current->size == total_required_size) {
                    break;
                }
            }
        }
        current = current->next;
    }

    if (!best_fit) {
        kerr_t err = heap_extend(total_required_size, &best_fit);
        if (k_failed(err)) {
            spinlock_release_irqrestore(&heap_lock, flags);
            return err;
        }
    }

    size_t min_split_size = HEAP_HEADER_SIZE + HEAP_MIN_PAYLOAD_SIZE;
    if (best_fit->size >= total_required_size + min_split_size) {
        heap_node_t *next_block = (heap_node_t *)((uintptr_t)best_fit + total_required_size);
        next_block->magic = HEAP_MAGIC_FREE;
        next_block->size = best_fit->size - total_required_size;
        next_block->payload_size = next_block->size - HEAP_HEADER_SIZE;

        next_block->next = best_fit->next;
        next_block->prev = best_fit;
        if (best_fit->next) {
            best_fit->next->prev = next_block;
        }
        best_fit->next = next_block;
        best_fit->size = total_required_size;
    }

    best_fit->magic = HEAP_MAGIC_USED;
    best_fit->payload_size = size;

    spinlock_release_irqrestore(&heap_lock, flags);

    *out = (uintptr_t)best_fit + HEAP_HEADER_SIZE;
    return E_OK;
}

/**
 * @brief Allocates and zeroes out a block of dynamic kernel memory.
 *
 * @param[in] size Requested size in bytes.
 * @param[out] out Pointer to store the virtual address of the allocated zeroed payload.
 * @return Returns E_OK on success, or an error code on failure.
 */
kerr_t kzalloc(size_t size, virt_addr_t *out) {
    virt_addr_t addr;
    K_TRY(kmalloc(size, &addr));
    memset((void *)addr, 0, size);
    *out = addr;
    return E_OK;
}

/**
 * @brief Frees a previously allocated block of kernel memory and coalesces adjacent free blocks.
 *
 * @param[in] addr Virtual address of the payload to free.
 * @return Returns E_OK on success, or an error code (E_BADADDR, E_INVAL) on failure.
 */
kerr_t kfree(virt_addr_t addr) {
    if (!addr)
        return E_BADADDR;

    irq_flags_t flags = spinlock_acquire_irqsave(&heap_lock);

    heap_node_t *block = (heap_node_t *)(addr - HEAP_HEADER_SIZE);

    if (block->magic != HEAP_MAGIC_USED) {
        spinlock_release_irqrestore(&heap_lock, flags);
        return E_INVAL;
    }

    block->magic = HEAP_MAGIC_FREE;
    block->payload_size = block->size - HEAP_HEADER_SIZE;

    // Coalesce Right
    if (block->next && block->next->magic == HEAP_MAGIC_FREE) {
        block->size += block->next->size;
        block->payload_size = block->size - HEAP_HEADER_SIZE;

        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Coalesce Left
    if (block->prev && block->prev->magic == HEAP_MAGIC_FREE) {
        block->prev->size += block->size;
        block->prev->payload_size = block->prev->size - HEAP_HEADER_SIZE;

        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }

    spinlock_release_irqrestore(&heap_lock, flags);
    return E_OK;
}

/** @brief Dumps the current state, blocks, and memory statistics of the heap manager to the serial log. */
void heap_dump() {
    irq_flags_t flags = spinlock_acquire_irqsave(&heap_lock);

    serial_printf("\n====================================== HEAP MANAGER =======================================\n");
    serial_printf("%-18s %-8s %-12s %-12s %-18s %-18s\n", "HEADER ADDR", "STATUS", "BLOCK SIZE", "PAYLOAD SZ", "PREV BLOCK", "NEXT BLOCK");
    serial_printf("-------------------------------------------------------------------------------------------\n");

    heap_node_t *current = heap_list_head;
    size_t block_count = 0;
    size_t total_free = 0;
    size_t total_used = 0;

    while (current != NULL) {
        const char *status = "UNKNOWN";
        if (current->magic == HEAP_MAGIC_FREE) {
            status = "FREE";
            total_free += current->size;
        } else if (current->magic == HEAP_MAGIC_USED) {
            status = "USED";
            total_used += current->size;
        } else {
            status = "CORRUPT";
        }

        serial_printf("%018llx %-8s %-12zu %-12zu %018llx %018llx\n", (unsigned long long)(uintptr_t)current, status, current->size, current->payload_size, (unsigned long long)(uintptr_t)current->prev, (unsigned long long)(uintptr_t)current->next);

        if (current->next == current) {
            serial_printf("HEAP: Circular link detected (next points to self)!\n");
            break;
        }

        current = current->next;
        block_count++;
    }

    serial_printf("-------------------------------------------------------------------------------------------\n");
    serial_printf("Summary: %zu Blocks | Free: %zu Bytes | Used: %zu Bytes | Total: %zu Bytes\n", block_count, total_free, total_used, total_free + total_used);
    serial_printf("===========================================================================================\n");

    spinlock_release_irqrestore(&heap_lock, flags);
}
