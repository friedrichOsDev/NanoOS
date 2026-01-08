#ifndef HEAP_H
#define HEAP_H

#include <stddef.h> 
#include <stdint.h>

#define HEAP_ALLOC_MAGIC 0xDEADBEEF
#define HEAP_FREE_MAGIC  0xCAFEBABE

typedef struct heap_block {
    size_t size;             
    struct heap_block* next; 
    uint32_t magic;          
} heap_block_t;

void heap_init(void* start_addr, size_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* kzalloc(size_t size);

#endif // HEAP_H