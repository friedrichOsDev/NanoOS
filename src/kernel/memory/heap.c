#include <heap.h>
#include <string.h>
#include <print.h>

#define MIN_ALLOC_SIZE (sizeof(heap_block_t))
#define HEAP_ALIGNMENT 4096

static heap_block_t* free_list_head = NULL;
static void* heap_start = NULL;
static size_t heap_total_size = 0;

// function to align a size
static size_t align_size(size_t size) {
    return (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
}

void heap_init(void* start_addr, size_t size) {
    if (!start_addr || size < MIN_ALLOC_SIZE) {
        printf("ERROR: heap_init received invalid parameters.\n");
        return;
    }

    // align the start address
    heap_start = (void*)align_size((size_t)start_addr);

    size_t aligned_start_diff = (size_t)heap_start - (size_t)start_addr;
    if (size <= aligned_start_diff) {
        printf("ERROR: Heap size too small after alignment adjustment.\n");
        return;
    }

    heap_total_size = size - aligned_start_diff;
    heap_total_size = heap_total_size & ~ (HEAP_ALIGNMENT - 1); // ensure total size is aligned

    if (heap_total_size < MIN_ALLOC_SIZE) {
        printf("ERROR: Heap total size %x is less than minimum alloc size after alignment.\n", (unsigned int)heap_total_size);
        return;
    }

    free_list_head = (heap_block_t*)heap_start;
    free_list_head->size = heap_total_size;
    free_list_head->next = NULL;
    free_list_head->magic = HEAP_FREE_MAGIC;

    printf("Heap initialized at %x with size %x.\n", (unsigned int)heap_start, (unsigned int)heap_total_size);
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t total_required_size = align_size(size + sizeof(heap_block_t));
    if (total_required_size < MIN_ALLOC_SIZE) {
        total_required_size = MIN_ALLOC_SIZE;
    }

    heap_block_t* current_block = free_list_head;
    heap_block_t* prev_block = NULL;

    while (current_block) {
        if (current_block->magic != HEAP_FREE_MAGIC) {
            printf("KERNEL PANIC: Heap corruption detected in kmalloc: invalid magic number on free list.\n");
            return NULL;
        }

        if (current_block->size >= total_required_size) {

            // check if we need to split the block
            if (current_block->size - total_required_size >= MIN_ALLOC_SIZE) {
                // split the block
                heap_block_t* new_free_block = (heap_block_t*)((char*)current_block + total_required_size);
                new_free_block->size = current_block->size - total_required_size;
                new_free_block->next = current_block->next;
                new_free_block->magic = HEAP_FREE_MAGIC;

                current_block->size = total_required_size;
                
                if (prev_block) {
                    prev_block->next = new_free_block;
                } else {
                    free_list_head = new_free_block;
                }
            } else {
                // not enough space to split, allocate the whole block
                if (prev_block) {
                    prev_block->next = current_block->next;
                } else {
                    free_list_head = current_block->next;
                }
            }
            current_block->magic = HEAP_ALLOC_MAGIC;

            // return pointer to data area
            return (void*)((char*)current_block + sizeof(heap_block_t));
        }

        prev_block = current_block;
        current_block = current_block->next;
    }

    printf("WARNING: kmalloc failed to allocate %x bytes.\n", (unsigned int)size);
    return NULL; // no block found
}

void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    // get the block header from the user pointer
    heap_block_t* block_to_free = (heap_block_t*)((char*)ptr - sizeof(heap_block_t));

    // integrity check: verify the magic number
    if (block_to_free->magic != HEAP_ALLOC_MAGIC) {
        printf("KERNEL PANIC: Heap corruption detected in kfree: invalid magic number. Double free or invalid pointer?\n");
        return;
    }

    heap_block_t* current = free_list_head;
    heap_block_t* prev = NULL;

    // find the correct insertion point to keep the free list sorted by address
    while (current != NULL && current < block_to_free) {
        if (current->magic != HEAP_FREE_MAGIC) {
             printf("KERNEL PANIC: Heap corruption detected in kfree: invalid magic number on free list.\n");
            return;
        }
        prev = current;
        current = current->next;
    }

    // mark the block as free
    block_to_free->magic = HEAP_FREE_MAGIC;

    // insert the freed block into the list
    if (prev == NULL) {
        free_list_head = block_to_free;
    } else {
        prev->next = block_to_free;
    }
    block_to_free->next = current;

    // --- Coalescing ---

    // 1. Coalesce with the *next* block if it's adjacent and free
    if (current != NULL && current->magic == HEAP_FREE_MAGIC && (char*)block_to_free + block_to_free->size == (char*)current) {
        block_to_free->size += current->size;
        block_to_free->next = current->next;
    }

    // 2. Coalesce with the *previous* block if it's adjacent and free
    if (prev != NULL && prev->magic == HEAP_FREE_MAGIC && (char*)prev + prev->size == (char*)block_to_free) {
        prev->size += block_to_free->size;
        prev->next = block_to_free->next;
    }
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}