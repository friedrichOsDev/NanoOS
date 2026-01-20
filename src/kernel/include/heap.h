/*
 * @file heap.h
 * @brief Header file for heap memory allocator implementation
 * @author friedrichOsDev
 */

#ifndef HEAP_H
#define HEAP_H

#include <stddef.h> 
#include <stdint.h>

/*
 * Structure representing a block in the heap
 */
typedef struct heap_block {
    size_t size;             
    struct heap_block* next; 
    uint32_t magic;          
} heap_block_t;

#define HEAP_ALLOC_MAGIC 0xDEADBEEF
#define HEAP_FREE_MAGIC  0xCAFEBABE
#define MIN_ALLOC_SIZE (sizeof(heap_block_t))
#define HEAP_ALIGNMENT 16
#define HEAP_PAGE_SIZE 4096

void heap_init(void);
void heap_setup(void* start_addr, size_t size);
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void* kmalloc_aligned(size_t size, size_t alignment);
void* kzalloc_aligned(size_t size, size_t alignment);
void kfree(void* ptr);

#endif // HEAP_H