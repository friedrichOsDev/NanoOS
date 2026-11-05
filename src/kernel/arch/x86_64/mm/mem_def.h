/**
 * @file mem_def.h
 * @brief Memory Management definitions and structures.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/* =========================================================================
 * General Page Definitions
 * ========================================================================= */

/** @brief Standard page size (4 KiB). */
#define PAGE_SIZE 0x1000
#define HUGE_PAGE_SIZE 0x200000

/** @brief Bitmask for isolating the page address. */
#define PAGE_MASK 0xFFFFFFFFFFFFF000ULL
#define HUGE_PAGE_MASK 0xFFFFFFFFFFE00000ULL

/* =========================================================================
 * Alignment Helper Macros
 * ========================================================================= */

/**
 * @brief Checks whether an address is aligned to a 4 KiB page boundary.
 * @param addr The address to check.
 */
#define IS_PAGE_ALIGNED(addr) (((uint64_t)(addr) & 0xFFF) == 0)

/**
 * @brief Rounds an address down to the nearest lower page boundary.
 * @param addr The address to round down.
 */
#define PAGE_ALIGN_DOWN(addr) ((uint64_t)(addr) & PAGE_MASK)

/**
 * @brief Rounds an address up to the nearest upper page boundary.
 * @param addr The address to round up.
 */
#define PAGE_ALIGN_UP(addr) (((uint64_t)(addr) + PAGE_SIZE - 1) & PAGE_MASK)

/* =========================================================================
 * Virtual Memory Layout (x86_64 Higher-Half)
 * ========================================================================= */

/** @brief Start address of the user-space range. */
#define USER_SPACE_START 0x0000000001000000ULL

/** @brief End address of the user-space range. */
#define USER_SPACE_END 0x00007FFFFFFFFFFFULL

/** @brief Beginning of the kernel address space. */
#define KERNEL_SPACE_START 0xFFFF800000000000ULL

/** @brief Start address of the direct-mapping region (physical memory mapped directly). */
#define DIRECT_MAPPING_START 0xFFFF800000000000ULL

/** @brief End address of the direct-mapping region. */
#define DIRECT_MAPPING_END 0xFFFF807FFFFFFFFFULL

/** @brief Start address of the dynamic kernel heap. */
#define KERNEL_HEAP_START 0xFFFF808000000000ULL

/** @brief End address of the dynamic kernel heap. */
#define KERNEL_HEAP_END 0xFFFF80FFFFFFFFFFULL

/** @brief Start address for the Memory-Mapped I/O (MMIO) region. */
#define MMIO_REGION_START 0xFFFF810000000000ULL

/** @brief End address for the Memory-Mapped I/O (MMIO) region. */
#define MMIO_REGION_END 0xFFFFFFFF7FFFFFFFULL

/** @brief Start address of the actual kernel executable code. */
#define KERNEL_CORE_START 0xFFFFFFFF80000000ULL

/**
 * @brief Converts a physical address into a virtual address within the direct-mapping region.
 * @param phys Source physical address.
 */
#define P2V(phys) ((virt_addr_t)(phys) + DIRECT_MAPPING_START)

/**
 * @brief Converts a virtual address within the direct-mapping region back into a physical address.
 * @param virt Source virtual address.
 */
#define V2P(virt) ((phys_addr_t)(virt) - DIRECT_MAPPING_START)

/* =========================================================================
 * Heap Definitions
 * ========================================================================= */

/** @brief Magic value for a free heap block. */
#define HEAP_MAGIC_FREE 0xDEADBEEF

/** @brief Magic value for an allocated heap block. */
#define HEAP_MAGIC_USED 0xCAFEBABE

/** @brief Size of the heap block header in bytes. */
#define HEAP_HEADER_SIZE sizeof(heap_node_t)

/** @brief Minimum payload size of a heap block in bytes. */
#define HEAP_MIN_PAYLOAD_SIZE 16

/** @brief Header structure for linked heap memory blocks. */
typedef struct heap_list {
    uint32_t magic;         /**< Status magic value (HEAP_MAGIC_FREE or HEAP_MAGIC_USED). */
    uint32_t _reserved;     /**< Reserved field for alignment. */
    uint64_t _align_pad;    /**< Alignment padding for 64-bit boundaries. */
    size_t size;            /**< Total size of the block (header + payload). */
    size_t payload_size;    /**< Pure payload size of the block. */
    struct heap_list *prev; /**< Pointer to the previous heap block. */
    struct heap_list *next; /**< Pointer to the next heap block. */
} heap_node_t;

/* =========================================================================
 * Paging Flags and Page Table Indices (x86_64 4-Level Paging)
 * ========================================================================= */

#define PTE_PRESENT (1ULL << 0)  /**< Page is present in memory. */
#define PTE_WRITABLE (1ULL << 1) /**< Write access allowed. */
#define PTE_USER (1ULL << 2)     /**< Access from user mode allowed. */
#define PTE_PWT (1ULL << 3)      /**< Page-level Write-Through. */
#define PTE_PCD (1ULL << 4)      /**< Page-level Cache Disable. */
#define PTE_ACCESSED (1ULL << 5) /**< Page has been accessed. */
#define PTE_DIRTY (1ULL << 6)    /**< Page has been written to. */
#define PTE_HUGE (1ULL << 7)     /**< Huge Page (2 MiB / 1 GiB). */
#define PTE_GLOBAL (1ULL << 8)   /**< Global page (not flushed from TLB on CR3 switch). */
#define PTE_NX (1ULL << 63)      /**< No-Execute (prevents code execution). */

/** @brief Bitmask for extracting the physical base address from a PTE. */
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL

/**
 * @brief Extracts the physical target address from a page table entry.
 * @param entry Value of the page table entry.
 */
#define PTE_GET_ADDR(entry) ((entry) & PTE_ADDR_MASK)

/** @brief Extracts the PML4 index from a virtual address. */
#define VMM_PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)

/** @brief Extracts the PDPT index from a virtual address. */
#define VMM_PDPT_INDEX(virt) (((virt) >> 30) & 0x1FF)

/** @brief Extracts the PD index from a virtual address. */
#define VMM_PD_INDEX(virt) (((virt) >> 21) & 0x1FF)

/** @brief Extracts the PT index from a virtual address. */
#define VMM_PT_INDEX(virt) (((virt) >> 12) & 0x1FF)

#define PT_MAX_ENTRIES 512
typedef uint64_t page_table_entry_t;

/** @brief Generic x86_64 page table structure for all 4 hierarchy levels (PML4, PDPT, PD, PT). */
typedef struct {
    page_table_entry_t entries[PT_MAX_ENTRIES]; /**< Array of 512 PTE entries. */
} __attribute__((aligned(PAGE_SIZE))) page_table_t;
