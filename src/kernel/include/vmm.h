/*
 * @file vmm.h
 * @brief Header file for Virtual Memory Manager (VMM)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pmm.h>

#define VMM_GET_DIR_INDEX(addr) ((addr) >> 22)
#define VMM_GET_TABLE_INDEX(addr) (((addr) >> 12) & 0x3FF)

#define VMM_PAGE_TABLE_ENTRIES 1024
#define VMM_PAGE_DIR_ENTRIES 1024
#define VMM_PAGE_SIZE PMM_PAGE_SIZE
#define VMM_RECURSIVE_SLOT (VMM_PAGE_DIR_ENTRIES - 1)
#define VMM_ZERO_SLOT (VMM_PAGE_DIR_ENTRIES - 2)
#define VMM_TABLES_BASE ((uintptr_t)VMM_RECURSIVE_SLOT << 22)
#define VMM_ZERO_WINDOW ((uintptr_t)VMM_ZERO_SLOT << 22)
#define VMM_PAGE_DIRECTORY_BASE (VMM_TABLES_BASE + (VMM_RECURSIVE_SLOT * VMM_PAGE_SIZE))
#define VMM_IS_ADDR_ALIGNED(addr) (((uint32_t)(addr) & (VMM_PAGE_SIZE - 1)) == 0)

#define VMM_PAGE_MASK 0xFFFFF000

#define VMM_PAGE_CACHE_DISABLED  0b00010000
#define VMM_PAGE_WRITE_THROUGH   0b00001000
#define VMM_PAGE_USER_SUPERVISOR 0b00000100
#define VMM_PAGE_READ_WRITE      0b00000010
#define VMM_PAGE_PRESENT         0b00000001

// higher half memory layout
#define VMM_USER_BASE            0x00000000
#define VMM_USER_END             0xBFFFFFFF
#define VMM_KERNEL_BASE          0xC0000000
#define VMM_KERNEL_END           0xCFFFFFFF
#define VMM_HEAP_START           0xD0000000
#define VMM_HEAP_END             0xDFFFFFFF
#define VMM_FRAMEBUFFER_BASE     0xE0000000
#define VMM_FRAMEBUFFER_END      0xEFFFFFFF
#define VMM_RESERVED_BASE        0xF0000000
#define VMM_RESERVED_END         VMM_ZERO_WINDOW - 1
#define VMM_ZERO_WINDOW_BASE     VMM_ZERO_WINDOW
#define VMM_RECURSIVE_BASE       VMM_TABLES_BASE

// define virt_addr_t
typedef uintptr_t virt_addr_t;

// page table structure
typedef struct {
    uint32_t entries[VMM_PAGE_TABLE_ENTRIES];
} page_table_t;

// page dir structure
typedef struct {
    uint32_t entries[VMM_PAGE_DIR_ENTRIES];
} page_directory_t;

#define VMM_GET_TABLE_ADDR(virt) ((page_table_t*)(VMM_TABLES_BASE + (VMM_GET_DIR_INDEX(virt) * VMM_PAGE_SIZE)))

extern void load_page_directory(phys_addr_t phys);
extern void enable_paging(void);
extern void disable_paging(void);

void vmm_prepare_zero_window(phys_addr_t phys, uint32_t window);
void vmm_init(void);
void vmm_map_page(page_directory_t* dir, virt_addr_t virtual_address, phys_addr_t physical_address, uint32_t flags);
void vmm_unmap_page(page_directory_t* dir, virt_addr_t virtual_address);
void vmm_map_pages(page_directory_t* dir, virt_addr_t virtual_start_address, phys_addr_t physical_start_address, uint32_t flags, uint32_t count);
void vmm_unmap_pages(page_directory_t* dir, virt_addr_t virtual_start_address, uint32_t count);
bool vmm_is_region_free(page_directory_t* dir, virt_addr_t start, uint32_t count);
phys_addr_t vmm_virtual_to_physical(page_directory_t* dir, virt_addr_t virtual_address);
page_directory_t* vmm_get_page_directory(void);
