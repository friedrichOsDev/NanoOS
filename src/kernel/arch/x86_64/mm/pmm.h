/**
 * @file pmm.h
 * @brief PMM definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Represents the global state and tracking metrics of the Physical Memory Manager (PMM). */
typedef struct {
    uint8_t *bitmap;      /**< Pointer to the physical memory allocation bitmap */
    uint64_t bitmap_size; /**< Size of the bitmap allocation in bytes */
    uint64_t total_pages; /**< Total number of physical memory pages managed */
    uint64_t used_pages;  /**< Number of currently allocated/used physical pages */
    uint64_t free_pages;  /**< Number of currently available/free physical pages */
} pmm_state_t;

extern pmm_state_t pmm_state;
