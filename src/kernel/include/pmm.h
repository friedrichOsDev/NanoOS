/*
 * @file pmm.h
 * @brief Header file for Physical Memory Manager (PMM)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PMM_PAGE_SIZE 4096
#define PMM_MAX_PHYS_ADDR 0xFFFFFFFF
#define PMM_ZERO_WINDOW_ADDR 0xFFFFE000
#define PMM_IS_PAGE_ALIGNED(addr) (((uint32_t)(addr) & (PMM_PAGE_SIZE - 1)) == 0)
#define PMM_ALIGN_UP(addr) (((addr) + PMM_PAGE_SIZE - 1) & ~(PMM_PAGE_SIZE - 1))
#define PMM_ALIGN_DOWN(addr) ((addr) & ~(PMM_PAGE_SIZE - 1))
#define PMM_BYTES_TO_PAGES(bytes) (((bytes) + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE)
#define PMM_PAGES_TO_BYTES(pages) ((pages) * PMM_PAGE_SIZE)
#define PMM_BITMAP_INDEX(addr) ((addr) / PMM_PAGE_SIZE / 8)
#define PMM_BITMAP_OFFSET(addr) (((addr) / PMM_PAGE_SIZE) % 8)

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
void pmm_lock_pages(phys_addr_t addr, size_t count);
void pmm_unlock_pages(phys_addr_t addr, size_t count);

bool pmm_is_page_free(phys_addr_t addr);

// statistics functions
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);
uint64_t pmm_get_total_memory(void);
pmm_state_t* pmm_get_state(void);
