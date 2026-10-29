/**
 * @file vmm.c
 * @brief Implementierung der virtuellen Speicherverwaltung (4-Level Paging x86_64).
 * @author friedrichOsDev
 */

#include "../apic/apic.h"
#include "mem_def.h"
#include "pmm.h"
#include <arch/arch.h>
#include <boot/multiboot2.h>
#include <core/init.h>
#include <kernel/panic.h>
#include <kernel/sync.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <pch.h>

/** @brief Physische Adresse der Kernel-PML4-Tabelle. */
phys_addr_t kernel_pml4_phys = 0;

/** @brief Virtuelle Adresse der Kernel-PML4-Tabelle. */
virt_addr_t kernel_pml4 = 0;

/** @brief Nächste freie virtuelle Adresse im reservierten MMIO-Bereich. */
static virt_addr_t next_free_mmio_vaddr = MMIO_REGION_START;

/** @brief Spinlock zur Absicherung von VMM-Paging-Operationen. */
static spinlock_t vmm_lock = SPINLOCK_INIT;

/** @brief Spinlock zur Absicherung der MMIO-Virtuelladressen-Allokation. */
static spinlock_t mmio_lock = SPINLOCK_INIT;

/**
 * @brief Liefert die Untertabelle der nächsten Paging-Ebene für einen gegebenen Index.
 *
 * Allokiert eine neue physische Seite über das PMM, falls die Tabelle noch nicht existiert.
 *
 * @param current_table Zeiger auf die aktuelle Seitentabelle (PML4, PDPT oder PD).
 * @param index Index innerhalb der Tabelle.
 * @param flags Zugriffs-Flags für neu erstellte Tabelleneinträge.
 * @return Zeiger (virtuelle Adresse) auf die Untertabelle der nächsten Ebene.
 */
static page_table_t *vmm_get_next_table(page_table_t *current_table,
                                        size_t index, uint64_t flags) {
    if (!current_table) {
        panic();
    }

    if (index >= PT_MAX_ENTRIES) {
        panic();
    }

    page_table_entry_t entry = current_table->entries[index];

    if (entry & PTE_PRESENT) {
        return (page_table_t *)P2V(PTE_GET_ADDR(entry));
    }

    phys_addr_t new_table_phys = pmm_page_alloc();
    if (!new_table_phys) {
        panic();
    }

    page_table_t *new_table_virt = (page_table_t *)P2V(new_table_phys);
    memset(new_table_virt, 0, PAGE_SIZE);

    current_table->entries[index] = new_table_phys | PTE_PRESENT | flags;

    return new_table_virt;
}

/**
 * @brief Prüft, ob eine Seitentabelle vollständig leer ist.
 *
 * @param table Zeiger auf die zu prüfende Seitentabelle.
 * @return `1` wenn keine gültigen Einträge (`PTE_PRESENT`) vorhanden sind, sonst `0`.
 */
static int vmm_is_table_empty(page_table_t *table) {
    for (size_t i = 0; i < PT_MAX_ENTRIES; i++) {
        if (table->entries[i] & PTE_PRESENT) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Hilfsfunktion zur sicheren Traversierung der Paging-Hierarchie.
 *
 * @param pml4 Zeiger auf die PML4-Tabelle.
 * @param pml4_idx Index in PML4.
 * @param pdpt Ausgabezeiger auf die PDPT-Tabelle.
 * @param pdpt_idx Index in PDPT.
 * @param pd Ausgabezeiger auf die PD-Tabelle.
 * @param pd_idx Index in PD.
 * @param pt Ausgabezeiger auf die PT-Tabelle.
 * @param pt_idx Index in PT.
 * @return `1` bei erfolgreicher Traversierung bis zur PT-Ebene, `0` falls ein Pfad nicht vorhanden ist.
 */
static int vmm_get_page_table_level(page_table_t *pml4, size_t pml4_idx, page_table_t **pdpt, size_t pdpt_idx, page_table_t **pd, size_t pd_idx, page_table_t **pt, size_t pt_idx) {
    (void)pt_idx;

    // PML4-Eintrag validieren
    if (!(pml4->entries[pml4_idx] & PTE_PRESENT)) {
        return 0;
    }

    phys_addr_t pdpt_phys = PTE_GET_ADDR(pml4->entries[pml4_idx]);
    if (!pdpt_phys) {
        return 0;
    }

    *pdpt = (page_table_t *)P2V(pdpt_phys);

    // PDPT-Eintrag validieren
    if (!((*pdpt)->entries[pdpt_idx] & PTE_PRESENT)) {
        return 0;
    }

    phys_addr_t pd_phys = PTE_GET_ADDR((*pdpt)->entries[pdpt_idx]);
    if (!pd_phys) {
        return 0;
    }

    *pd = (page_table_t *)P2V(pd_phys);

    // PD-Eintrag validieren
    if (!((*pd)->entries[pd_idx] & PTE_PRESENT)) {
        return 0;
    }

    phys_addr_t pt_phys = PTE_GET_ADDR((*pd)->entries[pd_idx]);
    if (!pt_phys) {
        return 0;
    }

    *pt = (page_table_t *)P2V(pt_phys);

    return 1;
}

void vmm_init() {
    kernel_pml4_phys = pmm_page_alloc();
    if (!kernel_pml4_phys) {
        panic();
    }

    page_table_t *k_pml4 = (page_table_t *)P2V(kernel_pml4_phys);
    memset(k_pml4, 0, PAGE_SIZE);

    uint64_t max_phys_ram = pmm_get_total_pages() * PAGE_SIZE;
    serial_printf("VMM: create direct mapping for %lld MiB of physical RAM\n", max_phys_ram / 1024 / 1024);
    for (uint64_t phys = 0; phys < max_phys_ram; phys += PAGE_SIZE) {
        vmm_map_page(vmm_kernel_space(), P2V(phys), phys, PTE_WRITABLE);
    }

    serial_printf("VMM: map kernel + bitmap\n");
    phys_addr_t kernel_phys_start = KERNEL_START_PHYS;
    uint64_t kernel_size = PAGE_ALIGN_UP(KERNEL_END_PHYS - kernel_phys_start);
    uint64_t total_higher_half_size = kernel_size + pmm_state.bitmap_size + 0x100000;
    for (uint64_t offset = 0; offset < total_higher_half_size; offset += PAGE_SIZE) {
        vmm_map_page(vmm_kernel_space(), KERNEL_CORE_START + kernel_phys_start + offset, kernel_phys_start + offset, PTE_WRITABLE);
    }

    // 16 MiB Identity-Mapping für frühe Bootphase/Hardware
    for (uint64_t addr = 0; addr < 0x1000000; addr += PAGE_SIZE) {
        vmm_map_page(vmm_kernel_space(), addr, addr, PTE_WRITABLE);
    }

    ASM_CODE("mov %0, %%cr3" ::"r"(kernel_pml4_phys) : "memory");

    vmm_unmap_page(vmm_kernel_space(), 0x0);

    serial_printf("VMM: map framebuffer via active paging\n");
    if (kernel_fb_info.fb_addr) {
        uint64_t fb_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
        virt_addr_t fb_vaddr = vmm_map_mmio(vmm_kernel_space(), kernel_fb_info.fb_addr, fb_size);
        if (!fb_vaddr) {
            panic();
        }
        kernel_fb_info.fb_addr = fb_vaddr;
    }

    kernel_pml4 = P2V(kernel_pml4_phys);
    serial_printf("VMM: init done, final pml4 tables active\n");
}

virt_addr_t vmm_map_mmio(vmm_space_t *sp, phys_addr_t paddr, size_t size) {
    if (size == 0)
        return 0;

    phys_addr_t phys_start = PAGE_ALIGN_DOWN(paddr);
    uint64_t offset = paddr - phys_start;
    size_t aligned_size = PAGE_ALIGN_UP(size + offset);

    uint64_t flags = spinlock_acquire_irqsave(&mmio_lock);
    if (next_free_mmio_vaddr + aligned_size > MMIO_REGION_END) {
        spinlock_release_irqrestore(&mmio_lock, flags);
        panic();
    }

    virt_addr_t assigned_vaddr = next_free_mmio_vaddr;
    next_free_mmio_vaddr += aligned_size;
    spinlock_release_irqrestore(&mmio_lock, flags);

    for (uint64_t i = 0; i < aligned_size; i += PAGE_SIZE) {
        uint64_t mmio_flags = PTE_WRITABLE | PTE_PCD | PTE_PWT;
        vmm_map_page(sp, assigned_vaddr + i, phys_start + i, mmio_flags);
    }

    return assigned_vaddr + offset;
}

void vmm_map_page(vmm_space_t *sp, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags) {
    if (!IS_PAGE_ALIGNED(vaddr))
        panic();
    if (!IS_PAGE_ALIGNED(paddr))
        panic();

    uint64_t lock_flags = spinlock_acquire_irqsave(&vmm_lock);

    size_t pml4_idx = VMM_PML4_INDEX(vaddr);
    size_t pdpt_idx = VMM_PDPT_INDEX(vaddr);
    size_t pd_idx = VMM_PD_INDEX(vaddr);
    size_t pt_idx = VMM_PT_INDEX(vaddr);

    uint64_t table_flags = PTE_WRITABLE | (flags & PTE_USER);

    page_table_t *pdpt = vmm_get_next_table((page_table_t *)(sp->pml4), pml4_idx, table_flags);
    if (!pdpt) {
        spinlock_release_irqrestore(&vmm_lock, lock_flags);
        panic();
    }

    page_table_t *pd = vmm_get_next_table(pdpt, pdpt_idx, table_flags);
    if (!pd) {
        spinlock_release_irqrestore(&vmm_lock, lock_flags);
        panic();
    }

    page_table_t *pt = vmm_get_next_table(pd, pd_idx, table_flags);
    if (!pt) {
        spinlock_release_irqrestore(&vmm_lock, lock_flags);
        panic();
    }

    if (pt->entries[pt_idx] & PTE_PRESENT) {
        spinlock_release_irqrestore(&vmm_lock, lock_flags);
        panic();
    }

    pt->entries[pt_idx] = (paddr & PAGE_MASK) | PTE_PRESENT | flags;

    ASM_CODE("invlpg (%0)" ::"r"(vaddr) : "memory");
    if (apic_initialized) {
        lapic_send_broadcast_tlb_ipi();
    }

    spinlock_release_irqrestore(&vmm_lock, lock_flags);
}

void vmm_unmap_page(vmm_space_t *sp, virt_addr_t vaddr) {
    if (vaddr == 0 || sp == NULL) {
        return;
    }
    if (!sp) {
        serial_printf("VMM ERROR: vmm_unmap_page called with NULL virt space!\n");
        return;
    }

    if (!IS_PAGE_ALIGNED(vaddr))
        panic();

    uint64_t lock_flags = spinlock_acquire_irqsave(&vmm_lock);

    size_t pml4_idx = VMM_PML4_INDEX(vaddr);
    size_t pdpt_idx = VMM_PDPT_INDEX(vaddr);
    size_t pd_idx = VMM_PD_INDEX(vaddr);
    size_t pt_idx = VMM_PT_INDEX(vaddr);

    // Hilfsfunktion zur sicheren Traversierung der Tabellen nutzen
    page_table_t *pdpt = NULL;
    page_table_t *pd = NULL;
    page_table_t *pt = NULL;

    if (!vmm_get_page_table_level((page_table_t *)(sp->pml4), pml4_idx, &pdpt, pdpt_idx, &pd, pd_idx, &pt, pt_idx)) {
        spinlock_release_irqrestore(&vmm_lock, lock_flags);
        return;
    }

    if (!(pt->entries[pt_idx] & PTE_PRESENT)) {
        spinlock_release_irqrestore(&vmm_lock, lock_flags);
        return;
    }

    pt->entries[pt_idx] = 0;
    ASM_CODE("invlpg (%0)" ::"r"(vaddr) : "memory");
    if (apic_initialized) {
        lapic_send_broadcast_tlb_ipi();
    }

    // Kaskadierendes Freigeben ungenutzter Seitentabellen
    if (vmm_is_table_empty(pt)) {
        phys_addr_t pt_phys = PTE_GET_ADDR(pd->entries[pd_idx]);
        pd->entries[pd_idx] = 0;
        pmm_page_free(pt_phys);

        if (vmm_is_table_empty(pd)) {
            phys_addr_t pd_phys = PTE_GET_ADDR(pdpt->entries[pdpt_idx]);
            pdpt->entries[pdpt_idx] = 0;
            pmm_page_free(pd_phys);

            if (vmm_is_table_empty(pdpt)) {
                phys_addr_t pdpt_phys = PTE_GET_ADDR(((page_table_t *)(sp->pml4))->entries[pml4_idx]);
                ((page_table_t *)(sp->pml4))->entries[pml4_idx] = 0;
                pmm_page_free(pdpt_phys);
            }
        }
    }

    spinlock_release_irqrestore(&vmm_lock, lock_flags);
}
