/**
 * @file pmm.h
 * @brief Physical Memory Management (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <stdint.h>

typedef struct {
    uint8_t *bitmap;
    uint64_t bitmap_size;
    uint64_t total_pages;
    uint64_t used_pages;
    uint64_t free_pages;
} pmm_state_t;

extern pmm_state_t pmm_state;

void pmm_init();
phys_addr_t pmm_page_alloc();
void pmm_page_free(phys_addr_t addr);