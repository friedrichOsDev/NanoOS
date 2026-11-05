/**
 * @file multiboot2.h
 * @brief Multiboot2 specification definitions and data structures.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Information regarding the linear graphics mode framebuffer. */
typedef struct {
    uint64_t fb_addr;   /**< Physical base address of the framebuffer */
    uint64_t fb_width;  /**< Screen width in pixels */
    uint64_t fb_height; /**< Screen height in pixels */
    uint64_t fb_pitch;  /**< Number of bytes per line (pitch) */
    uint64_t fb_bpp;    /**< Color depth in bits per pixel (BPP) */
} fb_info_t;

/** @brief Memory map region types according to the Multiboot2 specification. */
typedef enum {
    MMAP_USABLE = 1,           /**< Free, usable RAM */
    MMAP_RESERVED = 2,         /**< Reserved memory region */
    MMAP_ACPI_RECLAIMABLE = 3, /**< ACPI table area (reclaimable) */
    MMAP_NVS = 4,              /**< Non-volatile ACPI NVS storage */
    MMAP_BADRAM = 5            /**< Defective memory region */
} mmap_type_t;

/** @brief Single memory map entry. */
typedef struct {
    uint64_t base_addr; /**< Physical starting address */
    uint64_t length;    /**< Length of the memory region in bytes */
    mmap_type_t type;   /**< Type of the memory region */
} mmap_entry_t;

/** @brief Global memory map structure of the kernel. */
typedef struct {
    uint64_t entry_count;                   /**< Total number of recorded entries */
    mmap_entry_t entries[MMAP_MAX_ENTRIES]; /**< Array of all memory entries */
} mmap_t;

/** @brief Description of a module loaded by the bootloader. */
typedef struct {
    uint32_t mod_start; /**< Physical starting address of the module */
    uint32_t mod_end;   /**< Physical ending address of the module */
    char cmdline[3];    /**< Command line parameters for the module */
} boot_module_t;

/** @brief List of all modules provided by the bootloader. */
typedef struct {
    uint32_t count;                     /**< Total number of loaded modules */
    boot_module_t entries[MAX_MODULES]; /**< Module entries */
} boot_modules_t;

extern mmap_t kernel_mmap;
extern fb_info_t kernel_fb_info;
extern char kernel_cmdline[256];
extern char kernel_bootloader_name[64];
extern boot_modules_t kernel_modules;

void multiboot_parse(const uint64_t magic, const uint64_t info_ptr);
virt_addr_t get_rsdp_addr();
