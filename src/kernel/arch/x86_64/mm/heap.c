/**
 * @file heap.c
 * @brief Kernel HEAP allocator with splitting and bidirectional coalescing
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/pmm.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/panic.h>
#include <core/sync.h>
#include <lib/string.h>

static heap_list_t *heap_list_head = NULL;
static virt_addr_t heap_end_addr = 0;
static spinlock_t heap_lock = SPINLOCK_INIT;

/**
 * Initializes the HEAP allocator
 */
void heap_init() {
    phys_addr_t page1 = pmm_page_alloc();
    phys_addr_t page2 = pmm_page_alloc();
    phys_addr_t page3 = pmm_page_alloc();
    phys_addr_t page4 = pmm_page_alloc();
    if (!page1 || !page2 || !page3 || !page4) {
        panic("heap initial page allocation failed", 0);
    }

    vmm_map_page((page_table_t *)kernel_pml4, KERNEL_HEAP_START, page1,
                 PTE_WRITABLE);
    vmm_map_page((page_table_t *)kernel_pml4, KERNEL_HEAP_START + PAGE_SIZE,
                 page2, PTE_WRITABLE);
    vmm_map_page((page_table_t *)kernel_pml4, KERNEL_HEAP_START + PAGE_SIZE * 2,
                 page3, PTE_WRITABLE);
    vmm_map_page((page_table_t *)kernel_pml4, KERNEL_HEAP_START + PAGE_SIZE * 3,
                 page4, PTE_WRITABLE);

    memset((void *)KERNEL_HEAP_START, 0, PAGE_SIZE * 4);

    size_t initial_size = PAGE_SIZE * 4;
    heap_list_t *initial_block = (heap_list_t *)KERNEL_HEAP_START;
    initial_block->magic = HEAP_MAGIC_FREE;
    initial_block->size = initial_size;
    initial_block->payload_size = initial_size - HEAP_HEADER_SIZE;
    initial_block->prev = NULL;
    initial_block->next = NULL;

    heap_list_head = initial_block;
    heap_end_addr = KERNEL_HEAP_START + initial_size;

    serial_printf(COM1, "HEAP: initial_block at %llx with size %llx\n",
                  initial_block, initial_size);
    serial_printf(COM1, "HEAP: done\n");
}

/**
 * Extends the HEAP if kmalloc cannot find a suitable block for size
 * @param size Required size for the extension
 * @return Returns the new free block
 */
heap_list_t *heap_extend(size_t size) {
    size_t needed_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    virt_addr_t extension_start = heap_end_addr;

    for (size_t i = 0; i < needed_pages; i++) {
        phys_addr_t phys_page = pmm_page_alloc();
        if (!phys_page) {
            panic("heap out of physical memory during extension", 0);
        }

        vmm_map_page((page_table_t *)kernel_pml4,
                     extension_start + (i * PAGE_SIZE), phys_page,
                     PTE_WRITABLE);
    }

    size_t extended_bytes = needed_pages * PAGE_SIZE;

    memset((void *)extension_start, 0, extended_bytes);

    heap_list_t *new_block = (heap_list_t *)extension_start;
    new_block->magic = HEAP_MAGIC_FREE;
    new_block->size = extended_bytes;
    new_block->payload_size = extended_bytes - HEAP_HEADER_SIZE;
    new_block->prev = NULL;
    new_block->next = NULL;

    heap_end_addr += extended_bytes;

    heap_list_t *current = heap_list_head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = new_block;
    new_block->prev = current;

    if (current->magic == HEAP_MAGIC_FREE) {
        current->size += new_block->size;
        current->payload_size = current->size - HEAP_HEADER_SIZE;
        current->next = NULL;
        return current;
    }

    return new_block;
}

/**
 * Allocates memory from kernel HEAP
 * @param size Size to allocate
 * @return The virtual address of the allocated memory
 */
virt_addr_t kmalloc(size_t size) {
    if (size == 0)
        return 0;

    uint64_t flags = spinlock_acquire_irqsave(&heap_lock);

    size = (size + 7) & ~7;
    size_t total_required_size = size + HEAP_HEADER_SIZE;

    heap_list_t *current = heap_list_head;
    heap_list_t *best_fit = NULL;
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
        best_fit = heap_extend(total_required_size);
        if (!best_fit) {
            spinlock_release_irqrestore(&heap_lock, flags);
            panic("heap dynamic extension failed", size);
        }
    }

    size_t min_split_size = HEAP_HEADER_SIZE + HEAP_MIN_PAYLOAD_SIZE;
    if (best_fit->size >= total_required_size + min_split_size) {
        heap_list_t *next_block =
            (heap_list_t *)((uintptr_t)best_fit + total_required_size);
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

    return (uintptr_t)best_fit + HEAP_HEADER_SIZE;
}

/**
 * Allocates memory from kernel HEAP and zeros it
 * @param size Size to allocate
 * @return The virtual address of the allocated and zeroed memory
 */
virt_addr_t kzalloc(size_t size) {
    virt_addr_t addr = kmalloc(size);
    if (addr) {
        memset((void *)addr, 0, size);
    }
    return addr;
}

/**
 * Frees allocated memory
 * @param addr The virtual address of allocated memory
 */
void kfree(virt_addr_t addr) {
    if (!addr)
        return;

    uint64_t flags = spinlock_acquire_irqsave(&heap_lock);

    heap_list_t *block = (heap_list_t *)(addr - HEAP_HEADER_SIZE);

    if (block->magic != HEAP_MAGIC_USED) {
        serial_printf(
            COM1,
            "HEAP: Attempted kfree on invalid/already freed block at %llx!\n",
            addr);
        return;
    }

    block->magic = HEAP_MAGIC_FREE;
    block->payload_size = block->size - HEAP_HEADER_SIZE;

    // Coalesce Right (merge with next block if free)
    if (block->next && block->next->magic == HEAP_MAGIC_FREE) {
        block->size += block->next->size;
        block->payload_size = block->size - HEAP_HEADER_SIZE;

        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Coalesce Left (merge with previous block if free)
    if (block->prev && block->prev->magic == HEAP_MAGIC_FREE) {
        block->prev->size += block->size;
        block->prev->payload_size = block->prev->size - HEAP_HEADER_SIZE;

        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }

    spinlock_release_irqrestore(&heap_lock, flags);
}

/**
 * Dumps the current HEAP doubly-linked list and HEAP status
 */
void heap_dump(void) {
    uint64_t flags = spinlock_acquire_irqsave(&heap_lock);

    serial_printf(COM1, "\n====================================== HEAP MANAGER "
                        "=======================================\n");
    serial_printf(COM1, "%-18s %-8s %-12s %-12s %-18s %-18s\n", "HEADER ADDR",
                  "STATUS", "BLOCK SIZE", "PAYLOAD SZ", "PREV BLOCK",
                  "NEXT BLOCK");
    serial_printf(COM1, "------------------------------------------------------"
                        "-------------------------------------\n");

    heap_list_t *current = heap_list_head;
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

        serial_printf(COM1, "%018llx %-8s %-12zu %-12zu %018llx %018llx\n",
                      (unsigned long long)(uintptr_t)current, status,
                      current->size, current->payload_size,
                      (unsigned long long)(uintptr_t)current->prev,
                      (unsigned long long)(uintptr_t)current->next);

        if (current->next == current) {
            serial_printf(
                COM1, "HEAP: Circular link detected (next points to self)!\n");
            break;
        }

        current = current->next;
        block_count++;
    }

    serial_printf(COM1, "------------------------------------------------------"
                        "-------------------------------------\n");
    serial_printf(COM1,
                  "Summary: %zu Blocks | Free: %zu Bytes | Used: %zu Bytes | "
                  "Total: %zu Bytes\n",
                  block_count, total_free, total_used, total_free + total_used);
    serial_printf(COM1, "======================================================"
                        "=====================================\n\n");

    spinlock_release_irqrestore(&heap_lock, flags);
}