/**
 * @file _multiboot2.h
 * @brief Internal multiboot2 definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define MULTIBOOT2_MAGIC 0x36D76289
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_EFI_BS 12
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_EFI_64 17
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

/** @brief Main header of the Multiboot2 information structure. */
typedef struct {
    uint32_t total_size; /**< Total size of Multiboot2 info including all tags in bytes. */
    uint32_t reserved;   /**< Reserved field (must be zero). */
} __attribute__((packed)) multiboot_info_t;

/** @brief Generic header for all Multiboot2 tags. */
typedef struct {
    uint32_t type; /**< Tag type (MULTIBOOT_TAG_TYPE_*). */
    uint32_t size; /**< Total size of the tag in bytes. */
} __attribute__((packed)) multiboot_tag_t;

/** @brief Command line tag passed to the kernel. */
typedef struct {
    uint32_t type; /**< MULTIBOOT_TAG_TYPE_CMDLINE */
    uint32_t size; /**< Tag size */
    char string[]; /**< Null-terminated command line string */
} __attribute__((packed)) multiboot_tag_cmdline_t;

/** @brief Bootloader name and version tag. */
typedef struct {
    uint32_t type; /**< MULTIBOOT_TAG_TYPE_BOOT_LOADER */
    uint32_t size; /**< Tag size */
    char string[]; /**< Null-terminated bootloader name string */
} __attribute__((packed)) multiboot_tag_boot_loader_t;

/** @brief Bootloader modules tag (e.g., initrd, drivers). */
typedef struct {
    uint32_t type;      /**< MULTIBOOT_TAG_TYPE_MODULE */
    uint32_t size;      /**< Tag size */
    uint32_t mod_start; /**< Physical start address of module */
    uint32_t mod_end;   /**< Physical end address of module */
    char cmdline[];     /**< Optional module command line */
} __attribute__((packed)) multiboot_tag_module_t;

/** @brief Single entry in the Multiboot2 memory map. */
typedef struct {
    uint64_t base_addr; /**< Base address of memory region */
    uint64_t length;    /**< Length of memory region in bytes */
    uint32_t type;      /**< Memory type (1 = available, others reserved/ACPI/etc.) */
    uint32_t reserved;  /**< Reserved */
} __attribute__((packed)) multiboot_tag_mmap_entry_t;

/** @brief System memory map tag. */
typedef struct {
    uint32_t type;                        /**< MULTIBOOT_TAG_TYPE_MMAP */
    uint32_t size;                        /**< Tag size */
    uint32_t entry_size;                  /**< Size of a single entry */
    uint32_t entry_version;               /**< Entry structure version (currently 0) */
    multiboot_tag_mmap_entry_t entries[]; /**< Memory map entries array */
} __attribute__((packed)) multiboot_tag_mmap_t;

/** @brief Color component entry for palette-based framebuffers. */
typedef struct {
    uint8_t red;   /**< Red intensity (0–255) */
    uint8_t green; /**< Green intensity (0–255) */
    uint8_t blue;  /**< Blue intensity (0–255) */
} __attribute__((packed)) multiboot_tag_framebuffer_palette_t;

/** @brief Color information for palette-based framebuffers. */
typedef struct {
    uint32_t framebuffer_palette_num_colors;                   /**< Number of palette colors */
    multiboot_tag_framebuffer_palette_t framebuffer_palette[]; /**< Palette array */
} __attribute__((packed)) multiboot_tag_framebuffer_color_info_t;

/** @brief Bitfield layout for direct-color/RGB framebuffers. */
typedef struct {
    uint8_t framebuffer_red_field_position;   /**< Red bit offset */
    uint8_t framebuffer_red_mask_size;        /**< Red bit width */
    uint8_t framebuffer_green_field_position; /**< Green bit offset */
    uint8_t framebuffer_green_mask_size;      /**< Green bit width */
    uint8_t framebuffer_blue_field_position;  /**< Blue bit offset */
    uint8_t framebuffer_blue_mask_size;       /**< Blue bit width */
} __attribute__((packed)) multiboot_tag_framebuffer_rgb_info_t;

/** @brief Graphics framebuffer configuration tag. */
typedef struct {
    uint32_t type;               /**< MULTIBOOT_TAG_TYPE_FRAMEBUFFER */
    uint32_t size;               /**< Tag size */
    uint64_t framebuffer_addr;   /**< Physical base address */
    uint32_t framebuffer_pitch;  /**< Pitch (bytes per line) */
    uint32_t framebuffer_width;  /**< Width in pixels */
    uint32_t framebuffer_height; /**< Height in pixels */
    uint8_t framebuffer_bpp;     /**< Bits per pixel */
    uint8_t framebuffer_type;    /**< Framebuffer type (0 = indexed, 1 = RGB, 2 = text) */
    uint16_t reserved;           /**< Reserved */
    union {
        struct {
            uint16_t framebuffer_palette_num_colors;                    /**< Palette color count */
            multiboot_tag_framebuffer_palette_t framebuffer_palette[1]; /**< Color palette */
        };
        struct {
            uint8_t framebuffer_red_field_position;   /**< Red bit shift */
            uint8_t framebuffer_red_mask_size;        /**< Red bit length */
            uint8_t framebuffer_green_field_position; /**< Green bit shift */
            uint8_t framebuffer_green_mask_size;      /**< Green bit length */
            uint8_t framebuffer_blue_field_position;  /**< Blue bit shift */
            uint8_t framebuffer_blue_mask_size;       /**< Blue bit length */
        };
    };
} __attribute__((packed)) multiboot_tag_framebuffer_t;

/** @brief Tag for old ACPI v1.0 RSDP structure. */
typedef struct {
    uint32_t type;  /**< MULTIBOOT_TAG_TYPE_ACPI_OLD */
    uint32_t size;  /**< Tag size */
    uint8_t rsdp[]; /**< ACPI 1.0 RSDP raw data */
} __attribute__((packed)) multiboot_tag_old_acpi_t;

/** @brief Tag for new ACPI v2.0+ RSDP structure. */
typedef struct {
    uint32_t type;  /**< MULTIBOOT_TAG_TYPE_ACPI_NEW */
    uint32_t size;  /**< Tag size */
    uint8_t rsdp[]; /**< ACPI 2.0+ RSDP raw data */
} __attribute__((packed)) multiboot_tag_new_acpi_t;

/** @brief Tag for basic conventional memory information. */
typedef struct {
    uint32_t type;      /**< MULTIBOOT_TAG_TYPE_BASIC_MEMINFO */
    uint32_t size;      /**< Tag size */
    uint32_t mem_lower; /**< Lower memory in KiB (below 640 KiB) */
    uint32_t mem_upper; /**< Upper memory in KiB (above 1 MiB) */
} __attribute__((packed)) multiboot_tag_basic_meminfo_t;

/** @brief Tag for actual kernel physical load base address. */
typedef struct {
    uint32_t type;           /**< MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR */
    uint32_t size;           /**< Tag size */
    uint32_t load_base_addr; /**< Physical base address */
} __attribute__((packed)) multiboot_tag_load_base_addr_t;
