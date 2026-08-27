/**
 * @file init.h
 * @brief Kernel initialization code (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/boot/multiboot2.h>
#include <stdint.h>

extern uint8_t kernel_start[];
extern uint8_t kernel_end[];
extern uint8_t kernel_start_phys[];
extern uint8_t kernel_end_phys[];

#define KERNEL_START (uintptr_t) kernel_start
#define KERNEL_END (uintptr_t) kernel_end
#define KERNEL_START_PHYS (uintptr_t) kernel_start_phys
#define KERNEL_END_PHYS (uintptr_t) kernel_end_phys

#define MMAP_MAX_ENTRIES 1024

typedef struct {
    uint64_t fb_addr;
    uint64_t fb_width;
    uint64_t fb_height;
    uint64_t fb_pitch;
    uint64_t fb_bpp;
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
    uint64_t entry_count;
    mmap_entry_t entries[MMAP_MAX_ENTRIES];
} mmap_t;

extern mmap_t kernel_mmap;
extern fb_info_t kernel_fb_info;
extern multiboot_info_t *kernel_multiboot_info;
extern char kernel_cmdline[256];
extern char kernel_bootloader_name[64];

void kernel_init(uint64_t magic, uint64_t info_ptr);
