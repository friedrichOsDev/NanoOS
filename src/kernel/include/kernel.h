/**
 * @file kernel.h
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

/**
 * @brief Structure containing basic framebuffer information.
 */
typedef struct {
    void* fb_addr;      /**< Virtual address of the framebuffer. */
    uint32_t fb_width;  /**< Width in pixels. */
    uint32_t fb_height; /**< Height in pixels. */
    uint32_t fb_pitch;  /**< Number of bytes per scanline. */
    uint8_t fb_bpp;     /**< Bits per pixel. */
} fb_info_t;

/**
 * @brief Memory map entry types as defined by Multiboot2.
 */
typedef enum {
    MMAP_USABLE = 1,
    MMAP_RESERVED = 2,
    MMAP_ACPI_RECLAIMABLE = 3,
    MMAP_NVS = 4,
    MMAP_BADRAM = 5
} mmap_type_t;

/**
 * @brief A single entry in the kernel's internal memory map.
 */
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    mmap_type_t type;
} mmap_entry_t;

/**
 * @brief Internal memory map structure.
 */
typedef struct {
    uint32_t entry_count;
    mmap_entry_t entries[MMAP_MAX_ENTRIES];
} mmap_t;

/**
 * 
 */
typedef enum {
    INIT_START,
    INIT_SERIAL,
    INIT_GDT,
    INIT_IDT,
    INIT_IRQ,
    INIT_INTERRUPTS,
    INIT_MULTIBOOT,
    INIT_PMM,
    INIT_VMM,
    INIT_HEAP,
    INIT_TIMER,
    INIT_RTC,
    INIT_I8042,
    INIT_KBD,
    INIT_CONSOLE,
    INIT_SHELL,
    INIT_DONE
} init_state_t;

extern init_state_t init_state;
extern mmap_t kernel_mmap;
extern fb_info_t kernel_fb_info;
extern multiboot_info_t* kernel_multiboot_info;
extern char kernel_cmdline[256];
extern char kernel_bootloader_name[64];
