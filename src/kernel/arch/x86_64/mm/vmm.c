/**
 * @file vmm.c
 * @brief VMM inplementation.
 * @author friedrichOsDev
 */

#include "mem_def.h"
#include "pmm.h"
#include <arch/arch.h>
#include <arch/multiboot2.h>
#include <kernel/panic.h>
#include <kernel/sync.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <pch.h>

vmm_space_t kernel_space;
static virt_addr_t next_free_mmio_vaddr = MMIO_REGION_START;

/**
 * @brief Retrieves the next page table in the hierarchy, allocating a new one if it does not exist.
 *
 * @param[in] current_table Pointer to the current page table level.
 * @param[in] index Index within the current page table.
 * @param[in] flags Paging flags to apply if a new table is allocated.
 * @param[out] out Pointer to store the resolved virtual address of the next table.
 * @return Returns E_OK on success, or an error code (E_INVAL, E_NOMEM) on failure.
 */
static kerr_t vmm_get_next_table(page_table_t *current_table, size_t index, uint64_t flags, page_table_t **out) {
    if (!current_table) {
        return E_INVAL;
    }

    if (index >= PT_MAX_ENTRIES) {
        return E_NOMEM;
    }

    page_table_entry_t entry = current_table->entries[index];

    if (entry & PTE_PRESENT) {
        *out = (page_table_t *)P2V(PTE_GET_ADDR(entry));
        return E_OK;
    }

    phys_addr_t new_table_phys;
    K_TRY(pmm_page_alloc(&new_table_phys));

    page_table_t *new_table_virt = (page_table_t *)P2V(new_table_phys);
    memset(new_table_virt, 0, PAGE_SIZE);

    current_table->entries[index] = new_table_phys | PTE_PRESENT | flags;

    *out = new_table_virt;
    return E_OK;
}

/**
 * @brief Checks whether a given page table contains no present entries.
 *
 * @param[in] table Pointer to the page table to check.
 * @return Returns true if all entries are empty/not present, false otherwise.
 */
static bool vmm_is_table_empty(page_table_t *table) {
    for (size_t i = 0; i < PT_MAX_ENTRIES; i++) {
        if (table->entries[i] & PTE_PRESENT) {
            return false;
        }
    }
    return true;
}

// /**
//  * @brief Hilfsfunktion zur sicheren Traversierung der Paging-Hierarchie.
//  *
//  * @param pml4 Zeiger auf die PML4-Tabelle.
//  * @param pml4_idx Index in PML4.
//  * @param pdpt Ausgabezeiger auf die PDPT-Tabelle.
//  * @param pdpt_idx Index in PDPT.
//  * @param pd Ausgabezeiger auf die PD-Tabelle.
//  * @param pd_idx Index in PD.
//  * @param pt Ausgabezeiger auf die PT-Tabelle.
//  * @param pt_idx Index in PT.
//  * @return `1` bei erfolgreicher Traversierung bis zur PT-Ebene, `0` falls ein Pfad nicht vorhanden ist.
//  */
// static int vmm_get_page_table_level(page_table_t *pml4, size_t pml4_idx, page_table_t **pdpt, size_t pdpt_idx, page_table_t **pd, size_t pd_idx, page_table_t **pt, size_t pt_idx) {
//     (void)pt_idx;

//     // PML4-Eintrag validieren
//     if (!(pml4->entries[pml4_idx] & PTE_PRESENT)) {
//         return 0;
//     }

//     phys_addr_t pdpt_phys = PTE_GET_ADDR(pml4->entries[pml4_idx]);
//     if (!pdpt_phys) {
//         return 0;
//     }

//     *pdpt = (page_table_t *)P2V(pdpt_phys);

//     // PDPT-Eintrag validieren
//     if (!((*pdpt)->entries[pdpt_idx] & PTE_PRESENT)) {
//         return 0;
//     }

//     phys_addr_t pd_phys = PTE_GET_ADDR((*pdpt)->entries[pdpt_idx]);
//     if (!pd_phys) {
//         return 0;
//     }

//     *pd = (page_table_t *)P2V(pd_phys);

//     // PD-Eintrag validieren
//     if (!((*pd)->entries[pd_idx] & PTE_PRESENT)) {
//         return 0;
//     }

//     phys_addr_t pt_phys = PTE_GET_ADDR((*pd)->entries[pd_idx]);
//     if (!pt_phys) {
//         return 0;
//     }

//     *pt = (page_table_t *)P2V(pt_phys);

//     return 1;
// }

/**
 * @brief Initializes the Virtual Memory Manager (VMM), sets up the kernel address space, direct mapping, higher-half kernel, identity mappings, and maps the framebuffer.
 *
 * @return Returns E_OK on success, or an error code on failure.
 */
kerr_t vmm_init() {
    K_TRY(vmm_space_create(&kernel_space));

    page_table_t *k_pml4 = (page_table_t *)kernel_space.pml4;
    memset(k_pml4, 0, PAGE_SIZE);

    uint64_t max_phys_ram = pmm_get_total_pages() * PAGE_SIZE;
    uint64_t aligned_ram = (max_phys_ram + (uint64_t)HUGE_PAGE_SIZE - 1) & ~((uint64_t)HUGE_PAGE_SIZE - 1);

    log_debug("create direct mapping for %lld MiB of physical RAM", aligned_ram / 1024 / 1024);
    for (uint64_t phys = 0; phys < aligned_ram; phys += HUGE_PAGE_SIZE) {
        K_TRY(vmm_map_page(vmm_kernel_space(), P2V(phys), phys, PTE_WRITABLE | PTE_HUGE));
    }

    log_debug("map kernel + bitmap");
    phys_addr_t kernel_phys_start = KERNEL_START_PHYS;
    uint64_t kernel_size = PAGE_ALIGN_UP(KERNEL_END_PHYS - kernel_phys_start);
    uint64_t total_higher_half_size = kernel_size + pmm_state.bitmap_size + 0x100000;
    for (uint64_t offset = 0; offset < total_higher_half_size; offset += PAGE_SIZE) {
        K_TRY(vmm_map_page(vmm_kernel_space(), KERNEL_CORE_START + kernel_phys_start + offset, kernel_phys_start + offset, PTE_WRITABLE));
    }

    log_debug("map 16 MiB identity mapping");
    for (uint64_t addr = 0; addr < 0x1000000; addr += HUGE_PAGE_SIZE) {
        K_TRY(vmm_map_page(vmm_kernel_space(), addr, addr, PTE_WRITABLE | PTE_HUGE));
    }

    vmm_space_activate(&kernel_space);

    // err = vmm_unmap_page(vmm_kernel_space(), 0x0);
    // if (k_failed(err) && err != -2) {
    //     return err;
    // }
    // No clue, may be important

    // serial_printf("VMM: map framebuffer via active paging\n");
    // if (kernel_fb_info.fb_addr) {
    //     uint64_t fb_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
    //     virt_addr_t fb_vaddr = vmm_map_mmio(vmm_kernel_space(), kernel_fb_info.fb_addr, fb_size);
    //     if (!fb_vaddr) {
    //         panic();
    //     }
    //     kernel_fb_info.fb_addr = fb_vaddr;
    // }
    log_debug("map framebuffer");
    if (kernel_fb_info.fb_addr) {
        uint64_t raw_size = (uint64_t)kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;

        uint64_t huge_page_size = 0x200000; // 2 MiB
        uint64_t fb_size_aligned = (raw_size + huge_page_size - 1) & ~(huge_page_size - 1);

        virt_addr_t fb_vaddr = next_free_mmio_vaddr;
        next_free_mmio_vaddr += fb_size_aligned;

        for (uint64_t offset = 0; offset < fb_size_aligned; offset += huge_page_size) {
            phys_addr_t paddr = kernel_fb_info.fb_addr + offset;
            virt_addr_t vaddr = fb_vaddr + offset;

            uint64_t huge_flags = PTE_WRITABLE | PTE_PCD | PTE_PWT | PTE_HUGE;

            K_TRY(vmm_map_page(vmm_kernel_space(), vaddr, paddr, huge_flags));
        }

        kernel_fb_info.fb_addr = fb_vaddr;
    }
    return E_OK;
}

/**
 * @brief Maps a Memory-Mapped I/O (MMIO) region into the specified virtual address space.
 *
 * @param[in] sp Pointer to the virtual address space.
 * @param[in] paddr Physical base address of the MMIO region.
 * @param[in] size Size of the MMIO region in bytes.
 * @param[out] out Pointer to store the resulting virtual address.
 * @return Returns E_OK on success, or an error code (E_BADSIZE, E_NOMEM) on failure.
 */
kerr_t vmm_map_mmio(vmm_space_t *sp, phys_addr_t paddr, size_t size, virt_addr_t *out) {
    if (size == 0)
        return E_BADSIZE;

    phys_addr_t phys_start = PAGE_ALIGN_DOWN(paddr);
    uint64_t offset = paddr - phys_start;
    size_t aligned_size = PAGE_ALIGN_UP(size + offset);

    irq_flags_t flags = spinlock_acquire_irqsave(&(sp->lock));
    if (next_free_mmio_vaddr + aligned_size > MMIO_REGION_END) {
        spinlock_release_irqrestore(&(sp->lock), flags);
        return E_NOMEM;
    }

    virt_addr_t assigned_vaddr = next_free_mmio_vaddr;
    next_free_mmio_vaddr += aligned_size;
    spinlock_release_irqrestore(&(sp->lock), flags);

    for (uint64_t i = 0; i < aligned_size; i += PAGE_SIZE) {
        uint64_t mmio_flags = PTE_WRITABLE | PTE_PCD | PTE_PWT;
        K_TRY(vmm_map_page(sp, assigned_vaddr + i, phys_start + i, mmio_flags));
    }

    *out = assigned_vaddr + offset;
    return E_OK;
}

/**
 * @brief Maps a virtual address to a physical address within a given virtual address space.
 *
 * @param[in] sp Pointer to the virtual address space.
 * @param[in] vaddr Virtual address to map (must be page-aligned).
 * @param[in] paddr Physical address to map to (must be page-aligned).
 * @param[in] flags Paging flags to apply to the mapping entry.
 * @return Returns E_OK on success, or an error code on failure.
 */
kerr_t vmm_map_page(vmm_space_t *sp, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags) {
    if (!IS_PAGE_ALIGNED(vaddr))
        return E_BADADDR;
    if ((flags & PTE_HUGE) && (vaddr % HUGE_PAGE_SIZE != 0 || paddr % HUGE_PAGE_SIZE != 0))
        return E_BADADDR;
    else if (!IS_PAGE_ALIGNED(paddr))
        return E_BADADDR;

    irq_flags_t lock_flags = spinlock_acquire_irqsave(&(sp->lock));

    size_t pml4_idx = VMM_PML4_INDEX(vaddr);
    size_t pdpt_idx = VMM_PDPT_INDEX(vaddr);
    size_t pd_idx = VMM_PD_INDEX(vaddr);
    size_t pt_idx = VMM_PT_INDEX(vaddr);

    uint64_t table_flags = PTE_WRITABLE | (flags & PTE_USER);

    page_table_t *pdpt;
    kerr_t err = vmm_get_next_table((page_table_t *)(sp->pml4), pml4_idx, table_flags, &pdpt);
    if (k_failed(err)) {
        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return err;
    }

    page_table_t *pd;
    err = vmm_get_next_table(pdpt, pdpt_idx, table_flags, &pd);
    if (k_failed(err)) {
        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return err;
    }

    if (flags & PTE_HUGE) {
        if (pd->entries[pd_idx] & PTE_PRESENT) {
            spinlock_release_irqrestore(&(sp->lock), lock_flags);
            return E_NOTAVAIL;
        }
        pd->entries[pd_idx] = (paddr & HUGE_PAGE_MASK) | PTE_PRESENT | PTE_HUGE | flags;
    } else {
        page_table_t *pt;
        err = vmm_get_next_table(pd, pd_idx, table_flags, &pt);
        if (k_failed(err)) {
            spinlock_release_irqrestore(&(sp->lock), lock_flags);
            return err;
        }

        if (pt->entries[pt_idx] & PTE_PRESENT) {
            spinlock_release_irqrestore(&(sp->lock), lock_flags);
            return E_NOTAVAIL;
        }

        pt->entries[pt_idx] = (paddr & PAGE_MASK) | PTE_PRESENT | flags;
    }

    arch_tlb_flush_local(vaddr);
    if (is_apic_initialized()) {
        arch_tlb_shootdown();
    }

    spinlock_release_irqrestore(&(sp->lock), lock_flags);
    return err;
}

/**
 * @brief Unmaps a virtual address page from the given virtual address space and frees empty page tables.
 *
 * @param[in] sp Pointer to the virtual address space.
 * @param[in] vaddr Virtual address to unmap.
 * @return Returns E_OK on success, or an error code on failure.
 */
kerr_t vmm_unmap_page(vmm_space_t *sp, virt_addr_t vaddr) {
    if (vaddr == 0 || sp == NULL) {
        return E_INVAL;
    }

    if (!IS_PAGE_ALIGNED(vaddr))
        return E_BADADDR;

    irq_flags_t lock_flags = spinlock_acquire_irqsave(&(sp->lock));

    size_t pml4_idx = VMM_PML4_INDEX(vaddr);
    size_t pdpt_idx = VMM_PDPT_INDEX(vaddr);
    size_t pd_idx = VMM_PD_INDEX(vaddr);
    size_t pt_idx = VMM_PT_INDEX(vaddr);

    if (!(sp->pml4)) {
        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return E_INVAL;
    }

    page_table_t *pml4 = (page_table_t *)(sp->pml4);
    if (!(pml4->entries[pml4_idx] & PTE_PRESENT)) {
        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return E_NOTAVAIL;
    }

    page_table_t *pdpt = (page_table_t *)P2V(PTE_GET_ADDR(pml4->entries[pml4_idx]));
    if (!(pdpt->entries[pdpt_idx] & PTE_PRESENT)) {
        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return E_NOTAVAIL;
    }

    page_table_t *pd = (page_table_t *)P2V(PTE_GET_ADDR(pdpt->entries[pdpt_idx]));
    if (!(pd->entries[pd_idx] & PTE_PRESENT)) {
        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return E_NOTAVAIL;
    }

    if (pd->entries[pd_idx] & PTE_HUGE) {
        pd->entries[pd_idx] = 0;
        ASM_CODE("invlpg (%0)" ::"r"(vaddr) : "memory");
        if (is_apic_initialized()) {
            arch_tlb_shootdown();
        }

        if (vmm_is_table_empty(pd)) {
            phys_addr_t pd_phys = PTE_GET_ADDR(pdpt->entries[pdpt_idx]);
            pdpt->entries[pdpt_idx] = 0;
            K_TRY(pmm_page_free(pd_phys));

            if (vmm_is_table_empty(pdpt)) {
                phys_addr_t pdpt_phys = PTE_GET_ADDR(pml4->entries[pml4_idx]);
                pml4->entries[pml4_idx] = 0;
                K_TRY(pmm_page_free(pdpt_phys));
            }
        }

        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return E_OK;
    }

    page_table_t *pt = (page_table_t *)P2V(PTE_GET_ADDR(pd->entries[pd_idx]));
    if (!pt || !(pt->entries[pt_idx] & PTE_PRESENT)) {
        spinlock_release_irqrestore(&(sp->lock), lock_flags);
        return E_NOTAVAIL;
    }

    pt->entries[pt_idx] = 0;
    arch_tlb_flush_local(vaddr);
    if (is_apic_initialized()) {
        arch_tlb_shootdown();
    }

    if (vmm_is_table_empty(pt)) {
        phys_addr_t pt_phys = PTE_GET_ADDR(pd->entries[pd_idx]);
        pd->entries[pd_idx] = 0;
        K_TRY(pmm_page_free(pt_phys));

        if (vmm_is_table_empty(pd)) {
            phys_addr_t pd_phys = PTE_GET_ADDR(pdpt->entries[pdpt_idx]);
            pdpt->entries[pdpt_idx] = 0;
            K_TRY(pmm_page_free(pd_phys));

            if (vmm_is_table_empty(pdpt)) {
                phys_addr_t pdpt_phys = PTE_GET_ADDR(pml4->entries[pml4_idx]);
                pml4->entries[pml4_idx] = 0;
                K_TRY(pmm_page_free(pdpt_phys));
            }
        }
    }

    spinlock_release_irqrestore(&(sp->lock), lock_flags);
    return E_OK;
}

/**
 * @brief Returns a pointer to the global kernel virtual address space structure.
 *
 * @return Pointer to kernel space.
 */
vmm_space_t *vmm_kernel_space() {
    return &kernel_space;
}

/**
 * @brief Creates a new virtual address space, allocating its PML4 table and cloning kernel mappings.
 *
 * @param[out] out Pointer to store the created virtual address space.
 * @return Returns E_OK on success, or an error code on failure.
 */
kerr_t vmm_space_create(vmm_space_t *out) {
    phys_addr_t pml4_phys;
    K_TRY(pmm_page_alloc(&pml4_phys));

    vmm_space_t space;
    space.pml4_phys = pml4_phys;
    space.pml4 = P2V(pml4_phys);
    space.lock = (spinlock_t)SPINLOCK_INIT;

    page_table_t *new_pml4 = (page_table_t *)space.pml4;
    memset(new_pml4, 0, PAGE_SIZE);

    if (kernel_space.pml4) {
        page_table_t *k_pml4 = (page_table_t *)kernel_space.pml4;
        for (size_t i = 256; i < PT_MAX_ENTRIES; i++) {
            new_pml4->entries[i] = k_pml4->entries[i];
        }
    }

    *out = space;
    return E_OK;
}

/**
 * @brief Destroys a virtual address space, freeing all associated user page tables and its PML4 page.
 *
 * @param[in] sp Pointer to the virtual address space to destroy.
 * @return Returns E_OK on success, or an error code on failure.
 */
kerr_t vmm_space_destroy(vmm_space_t *sp) {
    if (!sp || sp == &kernel_space) {
        return E_INVAL;
    }

    page_table_t *pml4 = (page_table_t *)sp->pml4;
    if (!pml4) {
        return E_INVAL;
    }

    for (size_t i = 0; i < 256; i++) {
        if (!(pml4->entries[i] & PTE_PRESENT)) {
            continue;
        }

        phys_addr_t pdpt_phys = PTE_GET_ADDR(pml4->entries[i]);
        page_table_t *pdpt = (page_table_t *)P2V(pdpt_phys);

        for (size_t j = 0; j < PT_MAX_ENTRIES; j++) {
            if (!(pdpt->entries[j] & PTE_PRESENT)) {
                continue;
            }

            if (pdpt->entries[j] & PTE_HUGE) {
                continue;
            }

            phys_addr_t pd_phys = PTE_GET_ADDR(pdpt->entries[j]);
            page_table_t *pd = (page_table_t *)P2V(pd_phys);

            for (size_t k = 0; k < PT_MAX_ENTRIES; k++) {
                if (!(pd->entries[k] & PTE_PRESENT)) {
                    continue;
                }

                if (pd->entries[k] & PTE_HUGE) {
                    continue;
                }

                phys_addr_t pt_phys = PTE_GET_ADDR(pd->entries[k]);
                K_TRY(pmm_page_free(pt_phys)); // free PT
            }

            K_TRY(pmm_page_free(pd_phys)); // free PD
        }

        K_TRY(pmm_page_free(pdpt_phys)); // free PDPT
    }

    kerr_t err = pmm_page_free(sp->pml4_phys);
    return err;
}

/**
 * @brief Activates a virtual address space by loading its PML4 physical address into the CR3 register.
 *
 * @param[in] sp Pointer to the virtual address space to activate.
 */
void vmm_space_activate(vmm_space_t *sp) {
    ASM_CODE("mov %0, %%cr3" ::"r"(sp->pml4_phys) : "memory");
}
