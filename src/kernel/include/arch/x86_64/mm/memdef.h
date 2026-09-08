/**
 * @file memdef.h
 * @brief Memory Manager definitions
 * @author friedrichOsDev
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/* GENERAL PAGE DEFINITIONS */

#define PAGE_SIZE 0x1000
#define PAGE_MASK 0xFFFFFFFFFFFFF000ULL

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;

/* ALIGNMENT HELPER */

#define IS_PAGE_ALIGNED(addr) (((uint64_t)(addr) & 0xFFF) == 0)
#define ALIGN_DOWN(addr) ((uint64_t)(addr) & PAGE_MASK)
#define ALIGN_UP(addr) (((uint64_t)(addr) + PAGE_SIZE - 1) & PAGE_MASK)

/* VIRTUAL MEMORY LAYOUT */

#define USER_SPACE_START 0x0000000001000000ULL
#define USER_SPACE_END 0x00007FFFFFFFFFFFULL

#define KERNEL_SPACE_START 0xFFFF800000000000ULL
#define DIRECT_MAPPING_START 0xFFFF800000000000ULL
#define DIRECT_MAPPING_END 0xFFFF807FFFFFFFFFULL
#define KERNEL_HEAP_START 0xFFFF808000000000ULL
#define KERNEL_HEAP_END 0xFFFF80FFFFFFFFFFULL
#define MMIO_REGION_START 0xFFFF810000000000ULL
#define MMIO_REGION_END 0xFFFFFFFF7FFFFFFFULL
#define KERNEL_CORE_START 0xFFFFFFFF80000000ULL

#define P2V(phys) ((virt_addr_t)(phys) + DIRECT_MAPPING_START)
#define V2P(virt) ((phys_addr_t)(virt) - DIRECT_MAPPING_START)

/* HEAP DEFINITIONS */

#define HEAP_MAGIC_FREE 0xDEADBEEF
#define HEAP_MAGIC_USED 0xCAFEBABE
#define HEAP_HEADER_SIZE sizeof(heap_list_t)
#define HEAP_MIN_PAYLOAD_SIZE 16

typedef struct heap_list {
    uint32_t magic; // MAGIC_FREE or MAGIC_USED
    uint32_t _reserved;
    uint64_t _align_pad;
    size_t size;    // Header + Payload
    size_t payload_size;
    struct heap_list *prev;
    struct heap_list *next;
} heap_list_t;

/* PAGING FLAGS AND STRUCTURES */

#define PTE_PRESENT (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_USER (1ULL << 2)
#define PTE_PWT (1ULL << 3)
#define PTE_PCD (1ULL << 4)
#define PTE_ACCESSED (1ULL << 5)
#define PTE_DIRTY (1ULL << 6)
#define PTE_HUGE (1ULL << 7)
#define PTE_GLOBAL (1ULL << 8)
#define PTE_NX (1ULL << 63)

#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL
#define PTE_GET_ADDR(entry) ((entry) & PTE_ADDR_MASK)

#define VMM_PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)
#define VMM_PDPT_INDEX(virt) (((virt) >> 30) & 0x1FF)
#define VMM_PD_INDEX(virt) (((virt) >> 21) & 0x1FF)
#define VMM_PT_INDEX(virt) (((virt) >> 12) & 0x1FF)

#define PT_MAX_ENTRIES 512

typedef uint64_t page_table_entry_t;

/**
 * Generic Page Table structure for every level
 */
typedef struct {
    page_table_entry_t entries[PT_MAX_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;