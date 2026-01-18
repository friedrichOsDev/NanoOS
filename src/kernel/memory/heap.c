#include <heap.h>
#include <string.h>
#include <kernel.h>
#include <serial.h>

#define MIN_ALLOC_SIZE (sizeof(heap_block_t))
#define HEAP_ALIGNMENT 16
#define HEAP_PAGE_SIZE 4096

static heap_block_t* free_list_head = NULL;
static void* heap_start = NULL;
static size_t heap_total_size = 0;

extern char end[]; 

// function to align a size
static size_t align_size(size_t size) {
    return (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
}

void heap_init() {
    // find the largest available memory region for the heap
    uint32_t largest_region_base = 0;
    uint32_t largest_region_length = 0;
    
    uint32_t kernel_end = (uint32_t)end;
    // Align kernel_end to next page boundary to be safe

    kernel_end = (kernel_end + (HEAP_PAGE_SIZE - 1)) & ~(HEAP_PAGE_SIZE - 1);

    serial_puts("heap_init: kernel ends at 0x");
    serial_put_hex(kernel_end);
    serial_puts("\n");

    for (int i = 0; i < mmap_info->entry_count; i++) {
        mmap_entry_t* entry = &mmap_info->entries[i];

        if (entry->type == 1) { // type 1 = available RAM
            if (entry->base_addr_high == 0 && entry->length_high == 0) { // high bits must be 0 for 32-bit address space
                uint32_t base = entry->base_addr_low;
                uint32_t length = entry->length_low;

                // If the region overlaps with the kernel, adjust the base to start after the kernel
                if (base < kernel_end && (base + length) > kernel_end) {
                    uint32_t overlap = kernel_end - base;
                    base += overlap;
                    length -= overlap;
                }

                // ensure the memory region starts at or above 1MB to avoid low memory areas
                if (base >= 0x100000 && length > largest_region_length) {
                    largest_region_base = base;
                    largest_region_length = length;
                }
            }
        }
    }

    if (largest_region_base != 0 && largest_region_length != 0) {
        heap_setup((void*)largest_region_base, largest_region_length);
    } else {
        serial_puts("heap__init: no suitable memory region found.\n");
    }
}

void heap_setup(void* start_addr, size_t size) {
    if (!start_addr || size < MIN_ALLOC_SIZE) {
        serial_puts("heap_setup: invalid start address or size.\n");
        return;
    }

    // align the start address
    size_t start_val = (size_t)start_addr;
    heap_start = (void*)((start_val + HEAP_PAGE_SIZE - 1) & ~(HEAP_PAGE_SIZE - 1));

    size_t aligned_start_diff = (size_t)heap_start - (size_t)start_addr;
    if (size <= aligned_start_diff) {
        serial_puts("heap_setup: size too small after alignment.\n");
        return;
    }

    heap_total_size = size - aligned_start_diff;
    heap_total_size = heap_total_size & ~ (HEAP_ALIGNMENT - 1); // ensure total size is aligned

    if (heap_total_size < MIN_ALLOC_SIZE) {
        serial_puts("heap_setup: total size too small after alignment.\n");
        return;
    }

    free_list_head = (heap_block_t*)heap_start;
    free_list_head->size = heap_total_size;
    free_list_head->next = NULL;
    free_list_head->magic = HEAP_FREE_MAGIC;

    serial_puts("heap_setup: initialized at 0x");
    serial_put_hex((uint32_t)(size_t)heap_start);
    serial_puts(" with size 0x");
    serial_put_hex((uint32_t)heap_total_size);
    serial_puts(" (");
    serial_put_int(heap_total_size / 1024);
    serial_puts(" KB)\n");
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
            serial_puts("kmalloc: heap corruption detected in free list.\n");
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

    serial_puts("kmalloc: out of memory trying to allocate ");
    serial_put_int((int)size);
    serial_puts(" bytes.\n");
    
    return NULL; // no block found
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    if (size == 0) return;
    if ((alignment & (alignment - 1)) != 0) return; // Alignment must be power of 2

    size_t actual_size = align_size(size + sizeof(heap_block_t));
    
    heap_block_t* current = free_list_head;
    heap_block_t* prev = NULL;

    while (current) {
        if (current->magic != HEAP_FREE_MAGIC) {
            serial_puts("kmalloc_aligned: heap corruption detected.\n");
            return NULL;
        }

        size_t current_addr = (size_t)current;
        size_t data_addr = current_addr + sizeof(heap_block_t);
        
        size_t padding = 0;
        if (data_addr % alignment != 0) {
            padding = alignment - (data_addr % alignment);
        }
        
        size_t aligned_data_addr = data_addr + padding;
        size_t required_block_start = aligned_data_addr - sizeof(heap_block_t);
        size_t gap = required_block_start - current_addr;

        // If gap is too small to be a block, try next alignment
        if (gap > 0 && gap < MIN_ALLOC_SIZE) {
             aligned_data_addr += alignment;
             required_block_start = aligned_data_addr - sizeof(heap_block_t);
             gap = required_block_start - current_addr;
        }

        if (gap + actual_size <= current->size) {
            // Found fit
            
            // 1. Handle gap (split previous part)
            if (gap > 0) {
                heap_block_t* new_free = (heap_block_t*)((char*)current + gap);
                new_free->size = current->size - gap;
                new_free->next = current->next;
                new_free->magic = HEAP_FREE_MAGIC;
                
                current->size = gap;
                current->next = new_free;
                
                prev = current;
                current = new_free;
            }
            
            // 2. Allocate from current (which is now aligned)
            // Check if we need to split the rest
            if (current->size - actual_size >= MIN_ALLOC_SIZE) {
                heap_block_t* new_free = (heap_block_t*)((char*)current + actual_size);
                new_free->size = current->size - actual_size;
                new_free->next = current->next;
                new_free->magic = HEAP_FREE_MAGIC;
                
                current->size = actual_size;
                if (prev) prev->next = new_free;
                else free_list_head = new_free;
            } else {
                if (prev) prev->next = current->next;
                else free_list_head = current->next;
            }
            
            current->magic = HEAP_ALLOC_MAGIC;
            return (void*)((char*)current + sizeof(heap_block_t));
        }
        
        prev = current;
        current = current->next;
    }
    return NULL;
}

void* kzalloc_aligned(size_t size, size_t alignment) {
    void* ptr = kmalloc_aligned(size, alignment);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    // get the block header from the user pointer
    heap_block_t* block_to_free = (heap_block_t*)((char*)ptr - sizeof(heap_block_t));

    // integrity check: verify the magic number
    if (block_to_free->magic != HEAP_ALLOC_MAGIC) {
        serial_puts("kfree: invalid pointer or double free detected.\n");
        return;
    }

    heap_block_t* current = free_list_head;
    heap_block_t* prev = NULL;

    // find the correct insertion point to keep the free list sorted by address
    while (current != NULL && current < block_to_free) {
        if (current->magic != HEAP_FREE_MAGIC) {
            serial_puts("kfree: heap corruption detected in free list during insertion.\n");
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