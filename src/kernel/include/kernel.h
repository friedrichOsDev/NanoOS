/*
 * @file kernel.h
 * @brief Header file for kernel main source file
 * @author friedrichOsDev
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

/*
 * Memory map entry structure
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) mmap_entry_t;

/*
 * Memory map information structure
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    uint16_t entry_count;
    uint8_t reserved[2];
    mmap_entry_t entries[];
} __attribute__((packed)) mmap_info_t;

/*
 * Video block information structure
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    char signature[4];
    uint16_t version;
    uint32_t oem_string_ptr;
    uint32_t capabilities;
    uint32_t video_modes; 
    uint16_t video_memory;
    uint16_t software_rev;
    uint32_t vendor;
    uint32_t product_name;
    uint32_t product_rev;
    uint8_t reserved[222];
    uint8_t oem_data[256];
} __attribute__((packed)) video_block_info_t;

/*
 * VBE Mode Information Block structure
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;          
    uint16_t width;          
    uint16_t height;       
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;           
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t rsv_mask;
    uint8_t rsv_position;
    uint8_t direct_color_attributes;
    uint32_t framebuffer;    
    uint32_t offscreen_mem_off;
    uint16_t offscreen_mem_size;
    uint8_t reserved1[206];
} __attribute__((packed)) mode_info_block_t;

/*
 * VBE Screen Information structure
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint16_t bytes_per_line;
    uint32_t physical_buffer;
    uint32_t bytes_per_pixel;
    uint16_t x_cur_max;
    uint16_t y_cur_max;
} __attribute__((packed)) vbe_screen_info_t;

#define mmap_info   ((mmap_info_t*)0x8200)
#define video_info  ((video_block_info_t*)0x9000)
#define mode_info   ((mode_info_block_t*)(0x9000 + sizeof(video_block_info_t)))
#define screen_info ((vbe_screen_info_t*)(0x9000 + sizeof(video_block_info_t) + sizeof(mode_info_block_t)))

void kernel_main(void);

#endif // KERNEL_H
