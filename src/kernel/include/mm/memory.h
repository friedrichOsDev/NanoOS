/**
 * @file memory.h
 * @brief Memory management.
 * @author friedrichOsDev
 */

#pragma once

#include <kernel/sync.h>
#include <pch.h>

/** @brief Represents a virtual memory address space. */
typedef struct {
    virt_addr_t pml4;      /**< Virtual address of the PML4 table */
    phys_addr_t pml4_phys; /**< Physical address of the PML4 table */
    spinlock_t lock;       /**< Spinlock protecting the virtual address space */
} vmm_space_t;

/** @brief Virtual memory protection and attribute flags. */
typedef enum {
    VMM_READ = 1u << 0,   /**< Read permission */
    VMM_WRITE = 1u << 1,  /**< Write permission */
    VMM_EXEC = 1u << 2,   /**< Execute permission */
    VMM_USER = 1u << 3,   /**< User-mode access permission */
    VMM_DEVICE = 1u << 4, /**< Uncached memory flag for MMIO */
} vmm_prot_t;

kerr_t pmm_init();
kerr_t pmm_page_alloc(phys_addr_t *out);
kerr_t pmm_page_free(phys_addr_t addr);
uint64_t pmm_get_used_pages();
uint64_t pmm_get_free_pages();
uint64_t pmm_get_total_pages();

kerr_t vmm_init();
kerr_t vmm_map_mmio(vmm_space_t *sp, phys_addr_t paddr, size_t size, virt_addr_t *out);
kerr_t vmm_map_page(vmm_space_t *sp, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);
kerr_t vmm_unmap_page(vmm_space_t *sp, virt_addr_t vaddr);
vmm_space_t *vmm_kernel_space();
kerr_t vmm_space_create(vmm_space_t *out);
kerr_t vmm_space_destroy(vmm_space_t *sp);
void vmm_space_activate(vmm_space_t *sp); /* lädt CR3 */

kerr_t heap_init();
kerr_t kmalloc(size_t size, virt_addr_t *out);
kerr_t kzalloc(size_t size, virt_addr_t *out);
kerr_t kfree(virt_addr_t addr);
