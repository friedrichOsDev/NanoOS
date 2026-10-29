#pragma once

#include <pch.h>
#include <kernel/sync.h>

typedef struct {
    virt_addr_t pml4;
    phys_addr_t pml4_phys;
    spinlock_t lock;
} vmm_space_t;

typedef enum {
    VMM_READ   = 1u << 0,
    VMM_WRITE  = 1u << 1,
    VMM_EXEC   = 1u << 2,
    VMM_USER   = 1u << 3,
    VMM_DEVICE = 1u << 4,       /* uncached, für MMIO */
} vmm_prot_t;

void pmm_init();
phys_addr_t pmm_page_alloc();
void pmm_page_free(phys_addr_t addr);
uint64_t pmm_get_used_pages();
uint64_t pmm_get_free_pages();
uint64_t pmm_get_total_pages();

void vmm_init();
virt_addr_t vmm_map_mmio(vmm_space_t *sp, phys_addr_t paddr, size_t size);
void vmm_map_page(vmm_space_t *sp, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);
void vmm_unmap_page(vmm_space_t *sp, virt_addr_t vaddr);
vmm_space_t *vmm_kernel_space();
vmm_space_t *vmm_space_create();             /* NULL bei OOM */
void vmm_space_destroy(vmm_space_t *sp);
void vmm_space_activate(vmm_space_t *sp);    /* lädt CR3 */

void heap_init();
virt_addr_t kmalloc(size_t size);
virt_addr_t kzalloc(size_t size);
void kfree(virt_addr_t addr);
void heap_dump();
