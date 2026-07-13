/**
 * @file vmm.c
 * @brief Virtual Memory Management
 * @author friedrichOsDev
 */

#include <arch/x86_64/mm/vmm.h>
#include <arch/x86_64/mm/pmm.h>
#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/panic.h>
#include <lib/string.h>
#include <core/init.h>

phys_addr_t kernel_pml4_phys = 0;
static virt_addr_t next_free_mmio_vaddr = MMIO_REGION_START;

static page_table_t* vmm_get_next_table(page_table_t* current_table, size_t index, uint64_t flags) {
    page_table_entry_t entry = current_table->entries[index];

    if (entry & PTE_PRESENT) {
        return (page_table_t*)P2V(PTE_GET_ADDR(entry));
    }

    phys_addr_t new_table_phys = pmm_page_alloc();
    if (!new_table_phys) {
        panic("vnn out of physical memory creating page table", 0);
    }

    page_table_t* new_table_virt = (page_table_t*)P2V(new_table_phys);
    memset(new_table_virt, 0, PAGE_SIZE);

    current_table->entries[index] = new_table_phys | PTE_PRESENT | flags;

    return new_table_virt;
}

static int vmm_is_table_empty(page_table_t* table) {
    for (size_t i = 0; i < 512; i++) {
        if (table->entries[i] & PTE_PRESENT) {
            return 0;
        }
    }
    return 1;
}

void vmm_init() {
    kernel_pml4_phys = pmm_page_alloc();
    if (!kernel_pml4_phys) {
        panic("vmm out of physical memory creating kernel pml4", 0);
    }

    page_table_t* k_pml4 = (page_table_t*)P2V(kernel_pml4_phys);
    memset(k_pml4, 0, PAGE_SIZE);

    uint64_t max_phys_ram = pmm_state.total_pages * PAGE_SIZE;
    serial_printf(COM1, "VMM: create direct mapping for %lld MiB of physical RAM\n", max_phys_ram / 1024 / 1024);
    for (uint64_t phys = 0; phys < max_phys_ram; phys += PAGE_SIZE) {
        vmm_map_page(k_pml4, P2V(phys), phys, PTE_WRITABLE);
    }

    serial_printf(COM1, "VMM: map kernel + bitmap\n");
    phys_addr_t kernel_phys_start = KERNEL_START_PHYS;
    uint64_t kernel_size = ALIGN_UP(KERNEL_END_PHYS - kernel_phys_start);
    uint64_t total_higher_half_size = kernel_size + pmm_state.bitmap_size + 0x100000;
    for (uint64_t offset = 0; offset < total_higher_half_size; offset += PAGE_SIZE) {
        vmm_map_page(k_pml4, KERNEL_CORE_START + kernel_phys_start + offset, kernel_phys_start + offset, PTE_WRITABLE);
    }

    // 16 MiB identity mapping
    for (uint64_t addr = 0; addr < 0x1000000; addr += PAGE_SIZE) {
        vmm_map_page(k_pml4, addr, addr, PTE_WRITABLE);
    }
    vmm_unmap_page(k_pml4, 0x0);

    __asm__ __volatile__("mov %0, %%cr3" :: "r"(kernel_pml4_phys) : "memory");

    serial_printf(COM1, "VMM: map framebuffer via active paging\n");
    if (kernel_fb_info.fb_addr) {
        uint64_t fb_size = kernel_fb_info.fb_height * kernel_fb_info.fb_pitch;
        virt_addr_t fb_vaddr = vmm_map_mmio(k_pml4, kernel_fb_info.fb_addr, fb_size);
        if (!fb_vaddr) {
            panic("vmm failed to map UEFI framebuffer", 0);
        }
        kernel_fb_info.fb_addr = fb_vaddr;
    }

    serial_printf(COM1, "VMM: init done, final pml4 tables active\n");
}

virt_addr_t vmm_map_mmio(page_table_t *pml4, phys_addr_t paddr, size_t size) {
    if (size == 0) return 0;

    phys_addr_t phys_start = ALIGN_DOWN(paddr);
    uint64_t offset = paddr - phys_start;
    size_t aligned_size = ALIGN_UP(size + offset);

    if (next_free_mmio_vaddr + aligned_size > MMIO_REGION_END) {
        panic("vmm out of virtual memory for mmio region", 0);
    }

    virt_addr_t assigned_vaddr = next_free_mmio_vaddr;

    for (uint64_t i = 0; i < aligned_size; i += PAGE_SIZE) {
        uint64_t mmio_flags = PTE_WRITABLE | PTE_PCD | PTE_PWT;

        vmm_map_page(pml4, assigned_vaddr + i, phys_start + i, mmio_flags);
    }

    next_free_mmio_vaddr += aligned_size;

    return assigned_vaddr + offset;
}

void vmm_map_page(page_table_t *pml4, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags) {
    if (!IS_PAGE_ALIGNED(vaddr)) panic("vmm map unaligned vaddr", vaddr);
    if (!IS_PAGE_ALIGNED(paddr)) panic("vmm map unaligned paddr", paddr);

    size_t pml4_idx = VMM_PML4_INDEX(vaddr);
    size_t pdpt_idx = VMM_PDPT_INDEX(vaddr);
    size_t pd_idx   = VMM_PD_INDEX(vaddr);
    size_t pt_idx   = VMM_PT_INDEX(vaddr);

    uint64_t table_flags = PTE_WRITABLE | (flags & PTE_USER);

    page_table_t* pdpt = vmm_get_next_table(pml4, pml4_idx, table_flags);
    if (!pdpt) panic("vmm map failed at PDPT allocation", vaddr);

    page_table_t* pd   = vmm_get_next_table(pdpt, pdpt_idx, table_flags);
    if (!pd) panic("vmm map failed at PD allocation", vaddr);

    page_table_t* pt   = vmm_get_next_table(pd, pd_idx, table_flags);
    if (!pt) panic("vmm map failed at PT allocation", vaddr);

    pt->entries[pt_idx] = (paddr & PAGE_MASK) | PTE_PRESENT | flags;

    __asm__ __volatile__("invlpg (%0)" ::"r"(vaddr) : "memory");
}

void vmm_unmap_page(page_table_t *pml4, virt_addr_t vaddr) {
    if (!IS_PAGE_ALIGNED(vaddr)) panic("vmm unmap unaligned vaddr", vaddr);

    size_t pml4_idx = VMM_PML4_INDEX(vaddr);
    size_t pdpt_idx = VMM_PDPT_INDEX(vaddr);
    size_t pd_idx   = VMM_PD_INDEX(vaddr);
    size_t pt_idx   = VMM_PT_INDEX(vaddr);

    if (!(pml4->entries[pml4_idx] & PTE_PRESENT)) return;
    phys_addr_t pdpt_phys = PTE_GET_ADDR(pml4->entries[pml4_idx]);
    page_table_t* pdpt = (page_table_t*)P2V(pdpt_phys);

    if (!(pdpt->entries[pdpt_idx] & PTE_PRESENT)) return;
    phys_addr_t pd_phys = PTE_GET_ADDR(pdpt->entries[pdpt_idx]);
    page_table_t* pd = (page_table_t*)P2V(pd_phys);

    if (!(pd->entries[pd_idx] & PTE_PRESENT)) return;
    phys_addr_t pt_phys = PTE_GET_ADDR(pd->entries[pd_idx]);
    page_table_t* pt = (page_table_t*)P2V(pt_phys);

    pt->entries[pt_idx] = 0;
    __asm__ __volatile__("invlpg (%0)" ::"r"(vaddr) : "memory");

    if (vmm_is_table_empty(pt)) {
        pd->entries[pd_idx] = 0;
        pmm_page_free(pt_phys);

        if (vmm_is_table_empty(pd)) {
            pdpt->entries[pdpt_idx] = 0;
            pmm_page_free(pd_phys);

            if (vmm_is_table_empty(pdpt)) {
                pml4->entries[pml4_idx] = 0;
                pmm_page_free(pdpt_phys);
            }
        }
    }
}