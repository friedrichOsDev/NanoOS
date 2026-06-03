/**
 * @file heap.c
 * @author friedrichOsDev
 */

#include <heap.h>
#include <serial.h>
#include <string.h>
#include <print.h>
#include <panic.h>
#include <kernel.h>

static heap_block_t* heap_list = NULL;
static virt_addr_t current_heap_top;

/**
 * @brief Aligns a size to the heap alignment boundary.
 * @param size The size to align.
 * @return The aligned size.
 */
static inline size_t align_size(size_t size) {
    return (size + (HEAP_ALIGNMENT - 1)) & ~(HEAP_ALIGNMENT - 1);
}

/**
 * @brief Verifies the canary of a heap block and panics if corrupted.
 * @param block The block to verify.
 */
static void heap_verify_block(heap_block_t* block) {
    if (block->canary != HEAP_CANARY) {
        serial_printf("Heap: Corruption at block %x (Canary: %x, Expected: %x)\n", (virt_addr_t)block, block->canary, HEAP_CANARY);
        kernel_panic("Heap corruption detected: Invalid canary", (uint32_t)block);
    }
}

/**
 * @brief Initializes the kernel heap.
 * Maps initial pages and sets up the first free block.
 */
void heap_init(void) {
    size_t initial_map_size = (HEAP_INITIAL_PAGES + 1) * HEAP_PAGE_SIZE;

    // map initial heap pages (1 page buffer)
    vmm_map_pages(vmm_get_page_directory(), HEAP_START, pmm_zalloc_pages(HEAP_INITIAL_PAGES + 1), VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, HEAP_INITIAL_PAGES + 1);
    current_heap_top = HEAP_START + initial_map_size;

    heap_list = (heap_block_t*)HEAP_START;
    heap_list->canary = HEAP_CANARY;
    heap_list->size = initial_map_size - sizeof(heap_block_t);
    heap_list->magic = HEAP_MAGIC_FREE;
    heap_list->next = NULL;

    serial_printf("Heap: initial block at %x with size %d bytes\n", (virt_addr_t)heap_list, heap_list->size);
    init_state = INIT_HEAP;
}

/**
 * @brief Extends the heap by a given size.
 * @param size The minimum size to extend by.
 * @return true if successful, false otherwise.
 */
bool heap_extend(size_t size) {
    size_t pages_needed = (size + sizeof(heap_block_t) + HEAP_PAGE_SIZE - 1) / HEAP_PAGE_SIZE;
    virt_addr_t extend_base = current_heap_top;

    if (extend_base + (pages_needed * HEAP_PAGE_SIZE) > VMM_HEAP_END) {
        serial_printf("Heap: Error: Cannot extend heap by %d bytes (would exceed max heap size)\n", size);
        return false;
    }

    // map new pages
    for (size_t i = 0; i < pages_needed; i++) {
        phys_addr_t phys = pmm_zalloc_page();
        if (!phys) return false;

        vmm_map_page(vmm_get_page_directory(), extend_base + (i * HEAP_PAGE_SIZE), phys, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);
    }

    heap_block_t* last = heap_list;
    while (last && last->next) {
        heap_verify_block(last);
        last = last->next;
    }

    uintptr_t last_block_end = (uintptr_t)last + sizeof(heap_block_t) + last->size;

    if (last && last->magic == HEAP_MAGIC_FREE && last_block_end == extend_base) {
        last->size += (pages_needed * HEAP_PAGE_SIZE);
        current_heap_top += (pages_needed * HEAP_PAGE_SIZE);
        
        return true;
    }

    heap_block_t* new_block = (heap_block_t*)extend_base;
    new_block->canary = HEAP_CANARY;
    new_block->size = (pages_needed * HEAP_PAGE_SIZE) - sizeof(heap_block_t);
    new_block->magic = HEAP_MAGIC_FREE;
    new_block->next = NULL;

    if (last) {
        last->next = new_block;
    } else {
        heap_list = new_block;
    }

    current_heap_top += (pages_needed * HEAP_PAGE_SIZE);

    return true;
}

/**
 * @brief Allocates a block of memory from the heap.
 * @param size The number of bytes to allocate.
 * @return The virtual address of the allocated memory, or 0 on failure.
 */
virt_addr_t kmalloc(size_t size) {
    if (size == 0) {
        serial_printf("Heap: Error: Attempt to allocate zero bytes\n");
        return 0;
    }
    
    size_t size_aligned = align_size(size);

    heap_block_t* best_fit_block = NULL;
    heap_block_t* current_block = heap_list;

    // search for best fit free block
    while (current_block) {
        heap_verify_block(current_block);

        if (current_block->magic == HEAP_MAGIC_FREE && current_block->size >= size_aligned) {
            if (!best_fit_block || current_block->size < best_fit_block->size) {
                best_fit_block = current_block;
            }

            if (best_fit_block->size == size_aligned) break;
        }
        current_block = current_block->next;
    }

    if (!best_fit_block) {
        serial_printf("Heap: Warning: No suitable block found for size %d, extending heap...\n", size_aligned);
        if (heap_extend(size_aligned)) {
            return kmalloc(size);
        } else {
            serial_printf("Heap: Error: Failed to extend heap for size %d\n", size_aligned);
            return 0;
        }
    }

    // split block if there is enough space for a new block header and at least one alignment unit
    if (best_fit_block->size >= size_aligned + sizeof(heap_block_t) + HEAP_ALIGNMENT) {
        heap_block_t* new_block = (heap_block_t*)((uintptr_t)best_fit_block + sizeof(heap_block_t) + size_aligned);
        new_block->canary = HEAP_CANARY;
        new_block->size = best_fit_block->size - size_aligned - sizeof(heap_block_t);
        new_block->magic = HEAP_MAGIC_FREE;
        new_block->next = best_fit_block->next;

        best_fit_block->size = size_aligned;
        best_fit_block->next = new_block;
    }

    best_fit_block->magic = HEAP_MAGIC_ALLOCATED;
    return (virt_addr_t)((uintptr_t)best_fit_block + sizeof(heap_block_t));
}

/**
 * @brief Frees a previously allocated block of memory.
 * @param ptr The virtual address of the memory to free.
 */
void kfree(virt_addr_t ptr) {
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)((uintptr_t)ptr - sizeof(heap_block_t));
    heap_verify_block(block);

    if (block->magic != HEAP_MAGIC_ALLOCATED) {
        serial_printf("Heap: Error: Double free or invalid free at %x\n", ptr);
        return;
    }

    block->magic = HEAP_MAGIC_FREE;

    // Coalescing
    heap_block_t* curr = heap_list;
    while (curr != NULL) {
        heap_verify_block(curr);

        if (curr->magic == HEAP_MAGIC_FREE && curr->next != NULL && curr->next->magic == HEAP_MAGIC_FREE) {
            heap_verify_block(curr->next);

            uintptr_t curr_end = (uintptr_t)curr + sizeof(heap_block_t) + curr->size;
            if (curr_end == (uintptr_t)curr->next) {
                curr->size += sizeof(heap_block_t) + curr->next->size;
                curr->next = curr->next->next;
                continue;
            }
        }
        
        curr = curr->next;
    }
}

/**
 * @brief Allocates and zeroes a block of memory from the heap.
 * @param size The number of bytes to allocate.
 * @return The virtual address of the allocated memory, or 0 on failure.
 */
virt_addr_t kzalloc(size_t size) {
    virt_addr_t ptr = kmalloc(size);
    if (ptr) {
        memset((void*)ptr, 0, size);
    }
    return ptr;
}

/**
 * @brief Zeroes and then frees a block of memory.
 * @param ptr The virtual address of the memory to free.
 */
void kzfree(virt_addr_t ptr) {
    if (ptr == 0) return;
    heap_block_t* block = (heap_block_t*)(ptr - sizeof(heap_block_t));
    heap_verify_block(block);

    if (block->magic == HEAP_MAGIC_ALLOCATED) {
        memset((void*)ptr, 0, block->size);
    }
    kfree(ptr);
    
}

/**
 * @brief Reallocates a block of memory to a new size.
 * @param ptr The current virtual address of the memory.
 * @param new_size The new size in bytes.
 * @return The new virtual address, or 0 on failure.
 */
virt_addr_t krealloc(virt_addr_t ptr, size_t new_size) {
    if (ptr == 0) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return 0;
    }

    heap_block_t* block = (heap_block_t*)(ptr - sizeof(heap_block_t));
    heap_verify_block(block);

    if (block->magic != HEAP_MAGIC_ALLOCATED) {
        serial_printf("Heap: Error: Attempt to realloc invalid or free block at %x\n", ptr);
        return 0;
    }

    if (block->size >= new_size) {
        // Current block is already large enough
        return ptr;
    }

    virt_addr_t new_ptr = kmalloc(new_size);
    if (!new_ptr) return 0;

    memcpy((void*)new_ptr, (void*)ptr, block->size);
    kfree(ptr);
    return new_ptr;
}

/**
 * @brief Prints a debug dump of the current heap state to the serial port.
 */
void heap_dump(void) {
    char buf[128];
    serial_printf("\n--- Heap Dump ---\n");
    serial_printf("| #   | Address    | Size       | Status    | Next       |\n");
    serial_printf("|-----|------------|------------|-----------|------------|\n");

    heap_block_t* current = heap_list;
    uint32_t i = 0;
    while (current) {
        heap_verify_block(current);
        const char* status = (current->magic == HEAP_MAGIC_FREE) ? "FREE" : "ALLOCATED";
        snprintf(buf, sizeof(buf), "| %-3d | %010x | %-10d | %-9s | %010x |", i++, (uint32_t)current, current->size, status, (uint32_t)current->next);
        serial_printf("%s\n", buf);
        current = current->next;
    }
    
    serial_printf("|-----|------------|------------|-----------|------------|\n");
    serial_printf("--- End Heap Dump ---\n\n");
}

/**
 * @brief Returns the head of the heap block list.
 * @return Pointer to the first heap_block_t.
 */
heap_block_t* heap_get_list(void) {
    return heap_list;
}