/*
 * @file vmm.c
 * @brief Virtual Memory Manager (VMM)
 * @author friedrichOsDev
 */

#include <vmm.h>
#include <kernel.h>
#include <serial.h>
#include <string.h>
#include <panic.h>

extern uint32_t boot_page_table_zero_window[1024];
static page_directory_t* current_directory = NULL;

/*
 * Switch to a new page directory
 * @param dir The new page directory to switch to
 */
static inline void vmm_switch_directory(phys_addr_t phys_dir) {
    load_page_directory(phys_dir);
    current_directory = (page_directory_t*)(VMM_TABLES_BASE + VMM_RECURSIVE_SLOT * VMM_PAGE_SIZE);
}

/*
 * Reload the current page directory to flush the TLB
 * @param void
 */
static inline void vmm_reload_directory(void) {
    uint32_t cr3_val;
    __asm__ __volatile__(
        "mov %%cr3, %0\n\t"
        "mov %0, %%cr3"
        : "=r"(cr3_val)
        :
        : "memory"
    );
}

/*
 * Flush the TLB entry for a specific virtual address
 * @param addr The virtual address to flush from the TLB
 */
static inline void vmm_flush_tlb(virt_addr_t addr) {
    __asm__ __volatile__("invlpg (%0)" : : "r" (addr) : "memory");
}

/*
 * Prepare the zero page mapping for use in page clearing
 * @param phys The physical address to map to the zero page
 * @param window The index for the zero window
 */
void vmm_prepare_zero_window(phys_addr_t phys, uint32_t window) {
    if (window >= VMM_PAGE_TABLE_ENTRIES) {
        serial_printf("VMM: Error: Invalid zero window index %d\n", window);
        return;
    }
    if (current_directory == NULL) {
        // bootstrap mode
        boot_page_table_zero_window[window] = (phys & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
    } else {
        // normal mode
        page_table_t* zero_pt = VMM_GET_TABLE_ADDR(VMM_ZERO_WINDOW);
        zero_pt->entries[window] = (phys & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
    }

    vmm_flush_tlb(VMM_ZERO_WINDOW + (window * VMM_PAGE_SIZE));
}

/*
 * Initialize the Virtual Memory Manager (VMM)
 * @param void
 */
void vmm_init(void) {
    serial_printf("VMM: start\n");
    phys_addr_t page_dir_phys = pmm_alloc_page();
    phys_addr_t zero_table_phys = pmm_alloc_page();
    
    if (!page_dir_phys) kernel_panic("Failed to allocate initial page directory", 0);
    if (!zero_table_phys) kernel_panic("Failed to allocate zero page table", 0);

    // zero the page dir
    boot_page_table_zero_window[0] = (page_dir_phys & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
    boot_page_table_zero_window[1] = (zero_table_phys & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
    vmm_flush_tlb(VMM_ZERO_WINDOW);
    vmm_flush_tlb(VMM_ZERO_WINDOW + VMM_PAGE_SIZE);

    page_directory_t* working_dir = (page_directory_t*)(VMM_ZERO_WINDOW);
    page_table_t* zero_page_table = (page_table_t*)(VMM_ZERO_WINDOW + VMM_PAGE_SIZE);

    memset(working_dir, 0, VMM_PAGE_SIZE);
    memset(zero_page_table, 0, VMM_PAGE_SIZE);

    // initialize recursive mapping and zero window
    working_dir->entries[VMM_RECURSIVE_SLOT] = (page_dir_phys & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
    working_dir->entries[VMM_ZERO_SLOT] = (zero_table_phys & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;

    serial_printf("VMM: Debug: map kernel\n");
    phys_addr_t kernel_start_phys = (phys_addr_t)PMM_ALIGN_DOWN(KERNEL_START_PHYS);
    phys_addr_t kernel_end_phys = (phys_addr_t)PMM_ALIGN_UP(KERNEL_END_PHYS);
    virt_addr_t kernel_start_virt = (virt_addr_t)PMM_ALIGN_DOWN(KERNEL_START);
    uint32_t kernel_total_pages = (kernel_end_phys - kernel_start_phys) / VMM_PAGE_SIZE;
    vmm_map_pages(working_dir, kernel_start_virt, kernel_start_phys, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, kernel_total_pages);

    serial_printf("VMM: Debug: map bitmap\n");
    phys_addr_t bitmap_phys = (phys_addr_t)pmm_get_state()->bitmap; 
    phys_addr_t bitmap_start_phys = (phys_addr_t)PMM_ALIGN_DOWN(bitmap_phys);
    uint32_t bitmap_size = (pmm_get_state()->max_pages + 7) / 8;
    phys_addr_t bitmap_end_phys = (phys_addr_t)PMM_ALIGN_UP(bitmap_phys + bitmap_size);
    virt_addr_t bitmap_start_virt = (virt_addr_t)(PMM_ALIGN_UP(KERNEL_END) + VMM_PAGE_SIZE); // one page after the kernel
    uint32_t bitmap_total_pages = (bitmap_end_phys - bitmap_start_phys) / VMM_PAGE_SIZE;
    vmm_map_pages(working_dir, bitmap_start_virt, bitmap_start_phys, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, bitmap_total_pages);

    serial_printf("VMM: Debug: map framebuffer\n");
    phys_addr_t fb_phys = (phys_addr_t)kernel_fb_info.fb_addr;
    phys_addr_t fb_start_phys = (phys_addr_t)PMM_ALIGN_DOWN(fb_phys);
    phys_addr_t fb_end_phys = (phys_addr_t)PMM_ALIGN_UP(fb_phys + (kernel_fb_info.fb_height * kernel_fb_info.fb_pitch));
    virt_addr_t fb_start_virt = (virt_addr_t)VMM_FRAMEBUFFER_BASE;
    uint32_t fb_total_pages = (fb_end_phys - fb_start_phys) / VMM_PAGE_SIZE;
    vmm_map_pages(working_dir, fb_start_virt, fb_start_phys, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE | VMM_PAGE_CACHE_DISABLED | VMM_PAGE_WRITE_THROUGH, fb_total_pages);

    // clear boot zero window
    boot_page_table_zero_window[0] = 0;
    boot_page_table_zero_window[1] = 0;
    vmm_flush_tlb(VMM_ZERO_WINDOW);
    vmm_flush_tlb(VMM_ZERO_WINDOW + VMM_PAGE_SIZE);

    serial_printf("VMM: switching to new page directory at %x\n", page_dir_phys);
    vmm_switch_directory(page_dir_phys);

    // update bitmap addr
    virt_addr_t bitmap_addr_new = bitmap_start_virt + (bitmap_phys - bitmap_start_phys);
    pmm_get_state()->bitmap = (uint8_t*)bitmap_addr_new;

    // update framebuffer addr
    virt_addr_t fb_addr_new = fb_start_virt + (fb_phys - fb_start_phys);
    kernel_fb_info.fb_addr = (void*)fb_addr_new;

    serial_printf("VMM: done\n");
}

/*
 * Map a virtual address to a physical address in the given page directory
 * @param dir The page directory to modify
 * @param virtual_address The virtual address to map
 * @param physical_address The physical address to map to
 * @param flags The flags for the page (e.g., present, read/write, user/supervisor)
 */
void vmm_map_page(page_directory_t* dir, virt_addr_t virtual_address, phys_addr_t physical_address, uint32_t flags) {
    if (!dir) {
        serial_printf("VMM: Error: Attempt to map page with null page directory\n");
        return;
    }
    if (!VMM_IS_ADDR_ALIGNED(virtual_address)) {
        serial_printf("VMM: Error: Attempt to map page with unaligned virtual address %x\n", virtual_address);
        return;
    }
    if (!VMM_IS_ADDR_ALIGNED(physical_address)) {
        serial_printf("VMM: Error: Attempt to map page with unaligned physical address %x\n", physical_address);
        return;
    }

    vmm_map_pages(dir, virtual_address, physical_address, flags, 1);
}

/*
 * Unmap a virtual address in the given page directory
 * @param dir The page directory to modify
 * @param virtual_address The virtual address to unmap
 */
void vmm_unmap_page(page_directory_t* dir, virt_addr_t virtual_address) {
    if (!dir) {
        serial_printf("VMM: Error: Attempt to unmap page with null page directory\n");
        return;
    }
    if (!VMM_IS_ADDR_ALIGNED(virtual_address)) {
        serial_printf("VMM: Error: Attempt to unmap page with unaligned virtual address %x\n", virtual_address);
        return;
    }

    vmm_unmap_pages(dir, virtual_address, 1);
}

/*
 * Map a range of virtual addresses to physical addresses in the given page directory
 * @param dir The page directory to modify
 * @param virtual_start_address The virtual address to map
 * @param physical_start_address The physical address to map to
 * @param flags The flags for the page (e.g., present, read/write, user/supervisor)
 * @param count The number of pages to map
 */
void vmm_map_pages(page_directory_t* dir, virt_addr_t virtual_start_address, phys_addr_t physical_start_address, uint32_t flags, uint32_t count) {
    if (!dir) {
        serial_printf("VMM: Error: Attempt to map page with null page directory\n");
        return;
    }
    if (!VMM_IS_ADDR_ALIGNED(virtual_start_address)) {
        serial_printf("VMM: Error: Attempt to map pages with unaligned virtual start address %x\n", virtual_start_address);
        return;
    }
    if (!VMM_IS_ADDR_ALIGNED(physical_start_address)) {
        serial_printf("VMM: Error: Attempt to map pages with unaligned physical start address %x\n", physical_start_address);
        return;
    }
    if (count == 0 || count > pmm_get_state()->max_pages) {
        serial_printf("VMM: Error: Invalid page count %d for mapping\n", count);
        return;
    }

    if (!vmm_is_region_free(dir, virtual_start_address, count)) {
        if (!(virtual_start_address == VMM_ZERO_WINDOW)) {
            serial_printf("VMM: Error: Attempt to map pages to virtual address range %x - %x which is not free to map\n", virtual_start_address, virtual_start_address + (count * VMM_PAGE_SIZE));
            return;
            
        } else {
            if (count > 1) {
                serial_printf("VMM: Error: Attempt to map %d pages to zero window at virtual address %x which is only 1 page -> this would cause an overflow\n", count, virtual_start_address);
                return;
            }
            serial_printf("VMM: Warning: Mapping %d pages to zero window at virtual address %x which is currently mapped -> overwrite the existing mapping\n", count, virtual_start_address);
        }
        
    }

    bool reload_dir = false;
    if (count > 32) {
        reload_dir = true;
    }

    for (uint32_t i = 0; i < count; i++) {
        virt_addr_t cur_v = virtual_start_address + (i * VMM_PAGE_SIZE);
        phys_addr_t cur_p = physical_start_address + (i * VMM_PAGE_SIZE);
        uint32_t cur_dir_index = VMM_GET_DIR_INDEX(cur_v);
        uint32_t cur_table_index = VMM_GET_TABLE_INDEX(cur_v);

        page_table_t* table;
        if (dir->entries[cur_dir_index] & VMM_PAGE_PRESENT) {
            if (current_directory == NULL) {
                vmm_prepare_zero_window(dir->entries[cur_dir_index] & VMM_PAGE_MASK, 2);
                table = (page_table_t*)(VMM_ZERO_WINDOW + (2 * VMM_PAGE_SIZE));
            } else {
                table = VMM_GET_TABLE_ADDR(cur_v);
            }
        } else {
            phys_addr_t pt_phys = pmm_zalloc_page();
            dir->entries[cur_dir_index] = pt_phys | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
            vmm_flush_tlb(cur_v);
        if (current_directory == NULL) {
            vmm_prepare_zero_window(pt_phys, 3);
            table = (page_table_t*)(VMM_ZERO_WINDOW + (3 * VMM_PAGE_SIZE));
        } else {
            table = VMM_GET_TABLE_ADDR(cur_v);
            }
        }
        
        table->entries[cur_table_index] = (cur_p & VMM_PAGE_MASK) | flags | VMM_PAGE_PRESENT;
        if (!reload_dir) vmm_flush_tlb(cur_v);
    }

    if (reload_dir) vmm_reload_directory();
}

/*
 * Unmap a range of virtual addresses in the given page directory
 * @param dir The page directory to modify
 * @param virtual_start_address The virtual address to unmap
 * @param count The number of pages to unmap
 */
void vmm_unmap_pages(page_directory_t* dir, virt_addr_t virtual_start_address, uint32_t count) {
    if (!dir) {
        serial_printf("VMM: Error: Attempt to unmap page with null page directory\n");
        return;
    }
    if (!VMM_IS_ADDR_ALIGNED(virtual_start_address)) {
        serial_printf("VMM: Error: Attempt to unmap page with unaligned virtual address %x\n", virtual_start_address);
        return;
    }
    if (count == 0 || count > pmm_get_state()->max_pages) {
        serial_printf("VMM: Error: Invalid page count %d for unmapping\n", count);
        return;
    }

    bool reload_dir = false;
    if (count > 32) {
        reload_dir = true;
    }

    for (uint32_t i = 0; i < count; i++) {
        virt_addr_t cur_v = virtual_start_address + (i * VMM_PAGE_SIZE);
        uint32_t cur_dir_index = VMM_GET_DIR_INDEX(cur_v);
        uint32_t cur_table_index = VMM_GET_TABLE_INDEX(cur_v);

        page_table_t* table;
        if (dir->entries[cur_dir_index] & VMM_PAGE_PRESENT) {
            if (current_directory == NULL) {
                vmm_prepare_zero_window(dir->entries[cur_dir_index] & VMM_PAGE_MASK, 4);
                table = (page_table_t*)(VMM_ZERO_WINDOW + (4 * VMM_PAGE_SIZE));
            } else {
                table = VMM_GET_TABLE_ADDR(cur_v);
            }
        } else {
            serial_printf("VMM: Error: Attempt to unmap virtual address %x which is not mapped (page directory entry not present)\n", cur_v);
            continue;
        }
        table->entries[cur_table_index] = 0;
        if (!reload_dir) vmm_flush_tlb(cur_v);

        // test if the table is now empty
        bool empty = true;
        for (uint32_t j = 0; j < VMM_PAGE_TABLE_ENTRIES; j++) {
            if (table->entries[j] & VMM_PAGE_PRESENT) {
                empty = false;
            }
        }

        // delete table if empty
        if (empty) {
            pmm_free_page(dir->entries[cur_dir_index] & VMM_PAGE_MASK);
            dir->entries[cur_dir_index] = 0;
            reload_dir = true;
        }
    }

    if (reload_dir) vmm_reload_directory();
}

/*
 * Check if a range of virtual addresses is free (not mapped)
 * @param dir The page directory to check
 * @param start The starting virtual address
 * @param count The number of pages to check
 * @return True if the entire region is free, false otherwise
 */
bool vmm_is_region_free(page_directory_t* dir, virt_addr_t start, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        virt_addr_t cur_v = start + (i * VMM_PAGE_SIZE);
        uint32_t dir_index = VMM_GET_DIR_INDEX(cur_v);
        uint32_t table_index = VMM_GET_TABLE_INDEX(cur_v);

        if (!(dir->entries[dir_index] & VMM_PAGE_MASK)) {
            // skip to next dir entry
            i += (VMM_PAGE_TABLE_ENTRIES - table_index - 1);
            continue;
        } else {
            page_table_t* table;
            if (current_directory == NULL) {
                vmm_prepare_zero_window(dir->entries[dir_index] & VMM_PAGE_MASK, 5);
                table = (page_table_t*)(VMM_ZERO_WINDOW + (5 * VMM_PAGE_SIZE));
            } else {
                table = VMM_GET_TABLE_ADDR(cur_v);
            }

            if (table->entries[table_index] & VMM_PAGE_PRESENT) {
                return false;
            }
        }
    }
    return true;
}

/*
 * Get the physical address mapped to a virtual address
 * @param dir The page directory to search
 * @param virtual_address The virtual address to translate
 * @return The physical address, or 0 if not mapped
 */
phys_addr_t vmm_virtual_to_physical(page_directory_t* dir, virt_addr_t virtual_address) {
    if (!dir) {
        serial_printf("VMM: Error: Attempt to translate virtual address with null page directory\n");
        return 0;
    }

    bool is_aligned = VMM_IS_ADDR_ALIGNED(virtual_address);
    virt_addr_t virtual_address_aligned = virtual_address;
    if (!is_aligned) virtual_address_aligned = PMM_ALIGN_DOWN(virtual_address);
    uint32_t dir_index = VMM_GET_DIR_INDEX(virtual_address_aligned);
    uint32_t table_index = VMM_GET_TABLE_INDEX(virtual_address_aligned);

    if (dir->entries[dir_index] & VMM_PAGE_PRESENT) {
        page_table_t* table;
        if (current_directory == NULL) {
            vmm_prepare_zero_window(dir->entries[dir_index] & VMM_PAGE_MASK, 6);
            table = (page_table_t*)(VMM_ZERO_WINDOW + (6 * VMM_PAGE_SIZE));
        } else {
            table = VMM_GET_TABLE_ADDR(virtual_address_aligned);
        }
        if (table->entries[table_index] & VMM_PAGE_PRESENT) {
            if (is_aligned) {
                return (phys_addr_t)(table->entries[table_index] & VMM_PAGE_MASK);
            } else {
                uint32_t offset = virtual_address & ~VMM_PAGE_MASK;
                return (phys_addr_t)((table->entries[table_index] & VMM_PAGE_MASK) + offset);
            }
        }

        serial_printf("VMM: Warning: Attempt to translate virtual address %x which is not mapped (page table entry not present)\n", virtual_address);
        return 0;
    }

    serial_printf("VMM: Warning: Attempt to translate virtual address %x which is not mapped (page directory entry not present)\n", virtual_address);
    return 0;
}

/*
 * Get the current page directory
 * @param void
 * @return A pointer to the current page directory
 */
page_directory_t* vmm_get_page_directory(void) {
    return current_directory;
}