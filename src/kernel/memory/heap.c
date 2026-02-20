/*
 * @file heap.c
 * @brief Kernel heap
 * @author friedrichOsDev
 */

#include <heap.h>
#include <serial.h>
#include <string.h>

static heap_block_t* heap_list = NULL;
static virt_addr_t current_heap_top;

/*
 * Align a size up to the next multiple of HEAP_ALIGNMENT
 * @param size The size to align
 * @return The aligned size
 */
static inline size_t align_size(size_t size) {
    return (size + (HEAP_ALIGNMENT - 1)) & ~(HEAP_ALIGNMENT - 1);
}

/*
 * Initialize the kernel heap
 * @param void
 */
void heap_init(void) {
    serial_printf("Heap: start\n");

    size_t initial_map_size = (HEAP_INITIAL_PAGES + 1) * HEAP_PAGE_SIZE;

    // map initial heap pages (1 page buffer)
    vmm_map_pages(vmm_get_page_directory(), HEAP_START, pmm_zalloc_pages(HEAP_INITIAL_PAGES + 1), VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, HEAP_INITIAL_PAGES + 1);
    current_heap_top = HEAP_START + initial_map_size;

    heap_list = (heap_block_t*)HEAP_START;
    heap_list->size = initial_map_size - sizeof(heap_block_t);
    heap_list->magic = HEAP_MAGIC_FREE;
    heap_list->next = NULL;

    serial_printf("Heap: initial block at %x with size %d bytes\n", (virt_addr_t)heap_list, heap_list->size);
    serial_printf("Heap: done\n");
}

/*
 * Extend the heap by allocating more pages and adding them to the free list
 * @param size The minimum size to extend the heap by in bytes
 * @return true if the heap was successfully extended, false on failure
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
        phys_addr_t phys = pmm_alloc_page();
        if (!phys) return false;

        vmm_map_page(vmm_get_page_directory(), extend_base + (i * HEAP_PAGE_SIZE), phys, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);
    }

    heap_block_t* new_block = (heap_block_t*)extend_base;
    new_block->size = (pages_needed * HEAP_PAGE_SIZE) - sizeof(heap_block_t);
    new_block->magic = HEAP_MAGIC_FREE;
    new_block->next = NULL;

    heap_block_t* last = heap_list;
    while (last->next) last = last->next;
    last->next = new_block;

    current_heap_top += (pages_needed * HEAP_PAGE_SIZE);

    // coalesce free blocks
    heap_block_t* current = heap_list;
    while (current && current->next) {
        if (current->magic == HEAP_MAGIC_FREE && current->next->magic == HEAP_MAGIC_FREE) {
            uintptr_t current_end = (uintptr_t)current + sizeof(heap_block_t) + current->size;

            if (current_end == (uintptr_t)current->next) {
                // blocks are adjacent, coalesce
                current->size += sizeof(heap_block_t) + current->next->size;
                current->next = current->next->next;
                // Continue checking from the same block to see if the next one is also adjacent
                continue;
            }
        }
        current = current->next;
    }

    return true;
}

/*
 * Allocate a block of memory from the kernel heap
 * @param size The size of the block to allocate in bytes
 * @return The virtual address of the allocated block, or 0 on failure
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
        new_block->size = best_fit_block->size - size_aligned - sizeof(heap_block_t);
        new_block->magic = HEAP_MAGIC_FREE;
        new_block->next = best_fit_block->next;

        best_fit_block->size = size_aligned;
        best_fit_block->next = new_block;
    }

    best_fit_block->magic = HEAP_MAGIC_ALLOCATED;
    return (virt_addr_t)((uintptr_t)best_fit_block + sizeof(heap_block_t));
}

/*
 * Free a block of memory back to the kernel heap
 * @param ptr The virtual address of the block to free
 */
void kfree(virt_addr_t ptr) {
    if (ptr == 0) return;

    heap_block_t* block = (heap_block_t*)(ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC_ALLOCATED) {
        serial_printf("Heap: Error: Attempt to free invalid or already free block at %x\n", ptr);
        return;
    }

    block->magic = HEAP_MAGIC_FREE;

    // coalesce free blocks
    heap_block_t* current = heap_list;
    while (current && current->next) {
        if (current->magic == HEAP_MAGIC_FREE && current->next->magic == HEAP_MAGIC_FREE) {
            uintptr_t current_end = (uintptr_t)current + sizeof(heap_block_t) + current->size;

            if (current_end == (uintptr_t)current->next) {
                // blocks are adjacent, coalesce
                current->size += sizeof(heap_block_t) + current->next->size;
                current->next = current->next->next;
                // Continue checking from the same block to see if the next one is also adjacent
                continue;
            }
        }
        current = current->next;
    }
}

/*
 * Allocate a block of memory from the kernel heap and zero it
 * @param size The size of the block to allocate in bytes
 * @return The virtual address of the allocated block, or 0 on failure
 */
virt_addr_t kzalloc(size_t size) {
    virt_addr_t ptr = kmalloc(size);
    if (ptr) {
        memset((void*)ptr, 0, size);
    }
    return ptr;
}

/*
 * Free a block of memory back to the kernel heap and zero it
 * @param ptr The virtual address of the block to free
 */
void kzfree(virt_addr_t ptr) {
    if (ptr == 0) return;
    heap_block_t* block = (heap_block_t*)(ptr - sizeof(heap_block_t));
    if (block->magic == HEAP_MAGIC_ALLOCATED) {
        memset((void*)ptr, 0, block->size);
    }
    kfree(ptr);
    
}

void heap_dump(void) {
    heap_block_t* current = heap_list;
    uint32_t i = 0;
    size_t total_free = 0;
    size_t total_allocated = 0;

    serial_printf("\n--- HEAP DUMP START ---\n");
    serial_printf("Index | Address    | Status    | Size (Bytes) | Next\n");
    serial_printf("-------------------------------------------------------\n");

    while (current) {
        char* status = (current->magic == HEAP_MAGIC_ALLOCATED) ? "ALLOCATED" : "FREE     ";
        
        serial_printf("%d     | %x | %s | %d         | %x\n", 
                      i++, (uintptr_t)current, status, current->size, (uintptr_t)current->next);

        if (current->magic == HEAP_MAGIC_ALLOCATED) {
            total_allocated += current->size;
        } else {
            total_free += current->size;
        }

        current = current->next;
    }

    serial_printf("-------------------------------------------------------\n");
    serial_printf("Summary: Allocated: %d bytes, Free: %d bytes, Top: %x\n", 
                  total_allocated, total_free, current_heap_top);
    serial_printf("--- HEAP DUMP END ---\n\n");
}