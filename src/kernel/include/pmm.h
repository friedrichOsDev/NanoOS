/*
 * @file pmm.h
 * @brief Header file for Physical Memory Manager (PMM)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define PMM_MAX_PHYS_ADDR 0xFFFFFFFF
#define IS_PAGE_ALIGNED(addr) (((uint32_t)(addr) & 0xFFF) == 0)
#define ALIGN_UP(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define BYTES_TO_PAGES(bytes) (((bytes) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGES_TO_BYTES(pages) ((pages) * PAGE_SIZE)
#define BITMAP_INDEX(addr) ((addr) / PAGE_SIZE / 8)
#define BITMAP_OFFSET(addr) (((addr) / PAGE_SIZE) % 8)

typedef uintptr_t phys_addr_t;

typedef struct {
    uint8_t* bitmap;
    uint64_t max_pages;
    uint64_t used_pages;
    uint64_t last_checked_index;
} pmm_state_t; 

// initialization function
void pmm_init(void);

// allocation and deallocation functions
phys_addr_t pmm_alloc_page();
void pmm_free_page(phys_addr_t addr);
phys_addr_t pmm_zalloc_page();
void pmm_zfree_page(phys_addr_t addr);
phys_addr_t pmm_alloc_pages(size_t count);
void pmm_free_pages(phys_addr_t addr, size_t count);
phys_addr_t pmm_zalloc_pages(size_t count);
void pmm_zfree_pages(phys_addr_t addr, size_t count);

// memory protection functions
void pmm_lock_page(phys_addr_t addr);
void pmm_unlock_page(phys_addr_t addr);
void pmm_lock_pages(phys_addr_t addr, size_t count);
void pmm_unlock_pages(phys_addr_t addr, size_t count);

bool pmm_is_page_free(phys_addr_t addr);

// statistics functions
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);
uint64_t pmm_get_total_memory(void);
