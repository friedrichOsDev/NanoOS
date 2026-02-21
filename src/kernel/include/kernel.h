/*
 * @file kernel.h
 * @brief Header file for main entry point of the NanoOS kernel
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <multiboot2.h>

extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];
extern uint8_t _kernel_start_phys[];
extern uint8_t _kernel_end_phys[];

#define KERNEL_START ((uintptr_t)_kernel_start)
#define KERNEL_END ((uintptr_t)_kernel_end)
#define KERNEL_START_PHYS ((uintptr_t)_kernel_start_phys)
#define KERNEL_END_PHYS ((uintptr_t)_kernel_end_phys)

#define MMAP_MAX_ENTRIES 128

// simpler structures
typedef struct {
    void* fb_addr;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
    uint8_t fb_bpp;
} fb_info_t;

typedef enum {
    MMAP_USABLE = 1,
    MMAP_RESERVED = 2,
    MMAP_ACPI_RECLAIMABLE = 3,
    MMAP_NVS = 4,
    MMAP_BADRAM = 5
} mmap_type_t;

typedef struct {
    uint64_t base_addr;
    uint64_t length;
    mmap_type_t type;
} mmap_entry_t;

typedef struct {
    uint32_t entry_count;
    mmap_entry_t entries[MMAP_MAX_ENTRIES];
} mmap_t;

extern mmap_t kernel_mmap;
extern fb_info_t kernel_fb_info;
extern multiboot_info_t* kernel_multiboot_info;
