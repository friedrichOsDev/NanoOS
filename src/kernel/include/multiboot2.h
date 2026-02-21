/*
 * @file multiboot2.h
 * @brief Header file for Multiboot 2 structures and constants
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER 2
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8

typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed)) multiboot_info_t;

typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot_tag_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    char string[];
} __attribute__((packed)) multiboot_tag_cmdline_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    char string[];
} __attribute__((packed)) multiboot_tag_boot_loader_t;

typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed)) multiboot_tag_mmap_entry_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_tag_mmap_entry_t entries[];
} __attribute__((packed)) multiboot_tag_mmap_t;


// FB-TYPE = 0
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} __attribute__((packed)) multiboot_tag_framebuffer_palette_t;

typedef struct {
    uint32_t framebuffer_palette_num_colors;
    multiboot_tag_framebuffer_palette_t framebuffer_palette[];
} __attribute__((packed)) multiboot_tag_framebuffer_color_info_t;

// FB-TYPE = 1
typedef struct {
    uint8_t framebuffer_red_field_position;
    uint8_t framebuffer_red_mask_size;
    uint8_t framebuffer_green_field_position;
    uint8_t framebuffer_green_mask_size;
    uint8_t framebuffer_blue_field_position;
    uint8_t framebuffer_blue_mask_size;
} __attribute__((packed)) multiboot_tag_framebuffer_rgb_info_t;

// FB-TYPE = 2
// --> no additional color info

typedef struct {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
    uint8_t framebuffer_data[];
} __attribute__((packed)) multiboot_tag_framebuffer_t;
