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

static page_directory_t* current_directory;

/*
 * Switch to a new page directory
 * @param dir The new page directory to switch to
 */
static inline void vmm_switch_directory(page_directory_t* dir) {
    load_page_directory(dir);
    current_directory = dir;
}

/*
 * Flush the TLB entry for a specific virtual address
 * @param addr The virtual address to flush from the TLB
 */
static inline void vmm_flush_tlb(virt_addr_t addr) {
    __asm__ __volatile__("invlpg (%0)" : : "r" (addr) : "memory");
}

/*
 * Initialize the Virtual Memory Manager (VMM)
 * @param void
 */
void vmm_init(void) {
    serial_printf("VMM: start\n");
    current_directory = (page_directory_t*)pmm_zalloc_page();

    if (!current_directory) kernel_panic("Failed to allocate initial page directory", 0);

    // initialize recursive mapping
    current_directory->entries[VMM_RECURSIVE_SLOT] = ((phys_addr_t)current_directory & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;

    // initialize zero window
    page_table_t* zero_page_table = (page_table_t*)pmm_zalloc_page();
    if (!zero_page_table) kernel_panic("Failed to allocate zero page table", 0);
    current_directory->entries[VMM_ZERO_SLOT] = ((phys_addr_t)zero_page_table & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;

    // identity map the page dir, kernel, pmm bitmap, framebuffer and multiboot strukture
    serial_printf("VMM: identity mapping kernel, PMM bitmap, framebuffer and multiboot structure\n");

    vmm_map_page(current_directory, (virt_addr_t)current_directory, (phys_addr_t)current_directory, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);
    
    phys_addr_t kernel_start_addr = (phys_addr_t)PMM_ALIGN_DOWN(KERNEL_START_PHYS);
    phys_addr_t kernel_end_addr = (phys_addr_t)PMM_ALIGN_UP(KERNEL_END_PHYS);
    // for (phys_addr_t addr = PMM_ALIGN_DOWN(KERNEL_START_PHYS); addr < PMM_ALIGN_UP(KERNEL_END_PHYS); addr += VMM_PAGE_SIZE) vmm_map_page(current_directory, (virt_addr_t)addr, (phys_addr_t)addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);
    vmm_map_pages(current_directory, kernel_start_addr, kernel_start_addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, (kernel_end_addr - kernel_start_addr) / VMM_PAGE_SIZE);

    phys_addr_t bitmap_start_addr = (phys_addr_t)PMM_ALIGN_DOWN((phys_addr_t)pmm_get_state()->bitmap);
    phys_addr_t bitmap_end_addr = (phys_addr_t)PMM_ALIGN_UP((phys_addr_t)pmm_get_state()->bitmap + ((pmm_get_state()->max_pages + 7) / 8));
    // for (phys_addr_t addr = bitmap_start_addr; addr < bitmap_end_addr; addr += VMM_PAGE_SIZE) vmm_map_page(current_directory, (virt_addr_t)addr, (phys_addr_t)addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);
    vmm_map_pages(current_directory, bitmap_start_addr, bitmap_start_addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, (bitmap_end_addr - bitmap_start_addr) / VMM_PAGE_SIZE);

    phys_addr_t fb_start_addr = (phys_addr_t)PMM_ALIGN_DOWN((phys_addr_t)kernel_fb_info.fb_addr);
    phys_addr_t fb_end_addr = (phys_addr_t)PMM_ALIGN_UP((phys_addr_t)kernel_fb_info.fb_addr + (kernel_fb_info.fb_height * kernel_fb_info.fb_pitch));
    // for (phys_addr_t addr = fb_start_addr; addr < fb_end_addr; addr += VMM_PAGE_SIZE) vmm_map_page(current_directory, (virt_addr_t)addr, (phys_addr_t)addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);
    vmm_map_pages(current_directory, fb_start_addr, fb_start_addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, (fb_end_addr - fb_start_addr) / VMM_PAGE_SIZE);

    phys_addr_t multiboot_start_addr = (phys_addr_t)PMM_ALIGN_DOWN((phys_addr_t)kernel_multiboot_info);
    phys_addr_t multiboot_end_addr = (phys_addr_t)PMM_ALIGN_UP((phys_addr_t)kernel_multiboot_info + kernel_multiboot_info->total_size);
    // for (phys_addr_t addr = multiboot_start_addr; addr < multiboot_end_addr; addr += VMM_PAGE_SIZE) vmm_map_page(current_directory, (virt_addr_t)addr, (phys_addr_t)addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE);
    vmm_map_pages(current_directory, multiboot_start_addr, multiboot_start_addr, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, (multiboot_end_addr - multiboot_start_addr) / VMM_PAGE_SIZE);

    serial_printf("VMM: switching to new page directory at %x\n", (phys_addr_t)current_directory);
    vmm_switch_directory(current_directory);
    serial_printf("VMM: enabling paging\n");
    enable_paging();
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

    if (!paging_is_active()) {
        for (uint32_t i = 0; i < count; i++) {
            virt_addr_t cur_v = virtual_start_address + (i * VMM_PAGE_SIZE);
            phys_addr_t cur_p = physical_start_address + (i * VMM_PAGE_SIZE);
            uint32_t cur_dir_index = VMM_GET_DIR_INDEX(cur_v);
            uint32_t cur_table_index = VMM_GET_TABLE_INDEX(cur_v);

            page_table_t* table;
            if (dir->entries[cur_dir_index] & VMM_PAGE_PRESENT) {
                table = (page_table_t*)(dir->entries[cur_dir_index] & VMM_PAGE_MASK);
            } else {
                table = (page_table_t*)pmm_zalloc_page();
                dir->entries[cur_dir_index] = ((phys_addr_t)table & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
            }
            table->entries[cur_table_index] = (cur_p & VMM_PAGE_MASK) | flags | VMM_PAGE_PRESENT;
        }
        vmm_flush_tlb(virtual_start_address);
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        virt_addr_t cur_v = virtual_start_address + (i * VMM_PAGE_SIZE);
        phys_addr_t cur_p = physical_start_address + (i * VMM_PAGE_SIZE);
        uint32_t cur_dir_index = VMM_GET_DIR_INDEX(cur_v);
        uint32_t cur_table_index = VMM_GET_TABLE_INDEX(cur_v);

        page_table_t* table;
        if (dir->entries[cur_dir_index] & VMM_PAGE_PRESENT) {
            table = VMM_GET_TABLE_ADDR(cur_v);
        } else {
            table = (page_table_t*)pmm_zalloc_page();
            dir->entries[cur_dir_index] = ((phys_addr_t)table & VMM_PAGE_MASK) | VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE;
            vmm_flush_tlb(cur_v);
            table = VMM_GET_TABLE_ADDR(cur_v);
        }
        
        table->entries[cur_table_index] = (cur_p & VMM_PAGE_MASK) | flags | VMM_PAGE_PRESENT;
    }
    vmm_flush_tlb(virtual_start_address);
    return;
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

    if (!paging_is_active()) {
        for (uint32_t i = 0; i < count; i++) {
            virt_addr_t cur_v = virtual_start_address + (i * VMM_PAGE_SIZE);
            uint32_t cur_dir_index = VMM_GET_DIR_INDEX(cur_v);
            uint32_t cur_table_index = VMM_GET_TABLE_INDEX(cur_v);

            page_table_t* table;
            if (dir->entries[cur_dir_index] & VMM_PAGE_PRESENT) {
                table = (page_table_t*)(dir->entries[cur_dir_index] & VMM_PAGE_MASK);
            } else {
                serial_printf("VMM: Error: Attempt to unmap virtual address %x which is not mapped (page directory entry not present)\n", cur_v);
                continue;
            }
            table->entries[cur_table_index] = 0;

            // test if the table is now empty
            bool empty = true;
            for (uint32_t j = 0; j < VMM_PAGE_TABLE_ENTRIES; j++) {
                if (table->entries[j] & VMM_PAGE_PRESENT) {
                    empty = false;
                }
            }

            // delete table if empty
            if (empty) {
                pmm_free_page((phys_addr_t)table);
                dir->entries[cur_dir_index] = 0;
                serial_printf("VMM: Debug: SUCCESS deleting table\n");
            }
        }
        vmm_flush_tlb(virtual_start_address);
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        virt_addr_t cur_v = virtual_start_address + (i * VMM_PAGE_SIZE);
        uint32_t cur_dir_index = VMM_GET_DIR_INDEX(cur_v);
        uint32_t cur_table_index = VMM_GET_TABLE_INDEX(cur_v);

        page_table_t* table;
        if (dir->entries[cur_dir_index] & VMM_PAGE_PRESENT) {
            table = VMM_GET_TABLE_ADDR(cur_v);
        } else {
            serial_printf("VMM: Error: Attempt to unmap virtual address %x which is not mapped (page directory entry not present)\n", cur_v);
            continue;
        }
        table->entries[cur_table_index] = 0;

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
            serial_printf("VMM: Debug: SUCCESS deleting table\n");
        }
    }

    vmm_flush_tlb(virtual_start_address);
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

    if (!paging_is_active()) {
        return (phys_addr_t)virtual_address;
    }

    bool is_aligned = VMM_IS_ADDR_ALIGNED(virtual_address);
    virt_addr_t virtual_address_aligned = virtual_address;
    if (!is_aligned) virtual_address_aligned = PMM_ALIGN_DOWN(virtual_address);
    uint32_t dir_index = VMM_GET_DIR_INDEX(virtual_address_aligned);
    uint32_t table_index = VMM_GET_TABLE_INDEX(virtual_address_aligned);

    if (dir->entries[dir_index] & VMM_PAGE_PRESENT) {
        page_table_t* table = VMM_GET_TABLE_ADDR(virtual_address_aligned);
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

/*
 * Check if paging is currently active
 * @param void
 * @return True if paging is active, false otherwise
 */
bool paging_is_active(void) {
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r" (cr0));
    return (cr0 & 0x80000000) != 0;
}