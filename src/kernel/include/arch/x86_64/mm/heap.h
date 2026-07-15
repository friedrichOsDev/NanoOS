/**
 * @file heap.h
 * @brief Kernel HEAP allocator with splitting and bidirectional coalescing (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stddef.h>
#include <arch/x86_64/mm/memdef.h>

void heap_init();
heap_list_t* heap_extend(size_t size);
virt_addr_t kmalloc(size_t size);
virt_addr_t kzalloc(size_t size);
void kfree(virt_addr_t addr);
void heap_dump();