/**
 * @file pmm.c
 * @author friedrichOsDev
 */

#include <pmm.h>
#include <serial.h>
#include <kernel.h>
#include <panic.h>
#include <string.h>
#include <vmm.h>

static pmm_state_t pmm_state;
extern uint8_t boot_page_directory[];

/**
 * @brief Initializes the Physical Memory Manager.
 * 
 * Parses the multiboot memory map, determines the maximum physical address,
 * allocates and initializes the bitmap, and locks essential kernel regions.
 */
void pmm_init(void) {
    serial_printf("PMM: start\n");
    phys_addr_t max_addr = 0;

    // get the highest physical address from the memory map
    serial_printf("PMM: Parsing memory map with %d entries\n", kernel_mmap.entry_count);
    for (uint32_t i = 0; i < kernel_mmap.entry_count; i++) {
        if (kernel_mmap.entries[i].type == MMAP_USABLE) {
            uint64_t full_end = (uint64_t)kernel_mmap.entries[i].base_addr + kernel_mmap.entries[i].length;
            if (full_end > PMM_MAX_PHYS_ADDR) full_end = PMM_MAX_PHYS_ADDR;
            
            phys_addr_t end_addr = (phys_addr_t)full_end;
            if (end_addr > max_addr) max_addr = end_addr;
        }
    }

    pmm_state.max_pages = max_addr / PMM_PAGE_SIZE;
    uint32_t bitmap_size = (pmm_state.max_pages + 7) / 8;

    // search for space for the bitmaps in mmap
    pmm_state.bitmap = NULL;
    bool bitmap_found = false;
    for (uint32_t i = 0; i < kernel_mmap.entry_count; i++) {
        serial_printf("PMM: Checking block %d: base %x, len %x, needs %d\n", i, kernel_mmap.entries[i].base_addr, kernel_mmap.entries[i].length, bitmap_size);    
        if (kernel_mmap.entries[i].type == MMAP_USABLE && kernel_mmap.entries[i].length >= bitmap_size) {
            // cap at 4GB bound
            if (kernel_mmap.entries[i].base_addr + bitmap_size > PMM_MAX_PHYS_ADDR) continue;
            
            phys_addr_t candidate = (phys_addr_t)kernel_mmap.entries[i].base_addr;
            uint64_t block_end = kernel_mmap.entries[i].base_addr + kernel_mmap.entries[i].length;

            // Avoid placing bitmap at address 0 (NULL)
            if (candidate == 0) candidate += PMM_PAGE_SIZE;

            // Check for kernel overlap
            phys_addr_t kernel_start = PMM_ALIGN_DOWN(KERNEL_START_PHYS);
            phys_addr_t kernel_end = PMM_ALIGN_UP(KERNEL_END_PHYS);

            if (candidate < kernel_end && (candidate + bitmap_size) > kernel_start) {
                // Overlap detected, try to place after kernel
                if (candidate < kernel_end) candidate = kernel_end;
            }

            // Check if candidate still fits in the block
            if ((uint64_t)candidate + bitmap_size > block_end) continue;

            pmm_state.bitmap = (uint8_t*)candidate;
            bitmap_found = true;
            break;
        }
    }

    if (!bitmap_found) kernel_panic("Failed to find space for PMM bitmap", 0);
    if (bitmap_found) serial_printf("PMM: Bitmap placed at physical address %x, size %d bytes\n", (phys_addr_t)pmm_state.bitmap, bitmap_size);

    // initialize bitmap to all 1 (lock all pages)
    memset(pmm_state.bitmap, 0xFF, bitmap_size);
    pmm_state.used_pages = pmm_state.max_pages;
    pmm_state.last_checked_index = 0;

    // unlock usable mem
    for (uint32_t i = 0; i < kernel_mmap.entry_count; i++) {
        if (kernel_mmap.entries[i].type == MMAP_USABLE) {
            phys_addr_t start = PMM_ALIGN_UP(kernel_mmap.entries[i].base_addr);
            phys_addr_t end = PMM_ALIGN_DOWN(kernel_mmap.entries[i].base_addr + kernel_mmap.entries[i].length);
            pmm_unlock_pages(start, (end - start) / PMM_PAGE_SIZE);
        }
    }

    // lock kernel + boot page dir/table && bitmap && framebuffer && multiboot structure && (later acpi __TODO__)
    serial_printf("PMM: Locking kernel, bitmap, framebuffer, and multiboot structure\n");
    phys_addr_t kernel_start_aligned = PMM_ALIGN_DOWN(KERNEL_START_PHYS);
    phys_addr_t kernel_end_aligned = PMM_ALIGN_UP(KERNEL_END_PHYS);
    pmm_lock_pages(kernel_start_aligned, (kernel_end_aligned - kernel_start_aligned) / PMM_PAGE_SIZE);

    phys_addr_t boot_page_dir = PMM_ALIGN_DOWN((phys_addr_t)boot_page_directory);
    pmm_lock_pages(boot_page_dir, 3); // lock the page directory and tables

    phys_addr_t bitmap_start_aligned = PMM_ALIGN_DOWN((phys_addr_t)pmm_state.bitmap);
    phys_addr_t bitmap_end_aligned = PMM_ALIGN_UP((phys_addr_t)pmm_state.bitmap + bitmap_size);
    pmm_lock_pages(bitmap_start_aligned, (bitmap_end_aligned - bitmap_start_aligned) / PMM_PAGE_SIZE);

    if (kernel_fb_info.fb_addr) {
        phys_addr_t fb_start_aligned = PMM_ALIGN_DOWN((uintptr_t)kernel_fb_info.fb_addr);
        phys_addr_t fb_end_aligned = PMM_ALIGN_UP((uintptr_t)kernel_fb_info.fb_addr + (kernel_fb_info.fb_height * kernel_fb_info.fb_pitch));
        pmm_lock_pages(fb_start_aligned, (fb_end_aligned - fb_start_aligned) / PMM_PAGE_SIZE);
    }

    phys_addr_t multiboot_start_aligned = PMM_ALIGN_DOWN((phys_addr_t)kernel_multiboot_info);
    phys_addr_t multiboot_end_aligned = PMM_ALIGN_UP((phys_addr_t)kernel_multiboot_info + kernel_multiboot_info->total_size);
    pmm_lock_pages(multiboot_start_aligned, (multiboot_end_aligned - multiboot_start_aligned) / PMM_PAGE_SIZE);

    pmm_lock_pages(0x00000000, 256);

    serial_printf("PMM: Initialized with max address %x, total pages: %d\n", max_addr, pmm_state.max_pages);
    serial_printf("PMM: Free memory: %d KB, Used memory: %d KB\n", (uint32_t)(pmm_get_free_memory() / 1024), (uint32_t)(pmm_get_used_memory() / 1024));
    serial_printf("PMM: done\n");
}

/**
 * @brief Allocates a single physical page.
 * @return The physical address of the allocated page, or 0 on failure.
 */
phys_addr_t pmm_alloc_page() {
    return pmm_alloc_pages(1);
}

/**
 * @brief Frees a single physical page.
 * @param addr The physical address of the page to free.
 */
void pmm_free_page(phys_addr_t addr) {
    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Attempt to free unaligned page at address %x\n", addr);
        return;
    }
    
    pmm_free_pages(addr, 1);
}

/**
 * @brief Allocates and zeroes a single physical page.
 * @return The physical address of the allocated page, or 0 on failure.
 */
phys_addr_t pmm_zalloc_page() {
    phys_addr_t addr = pmm_alloc_page();
    if (!addr) {
        serial_printf("PMM: Error: Failed to allocate page\n");
        return 0;
    }

    vmm_prepare_zero_window(addr, 7);
    memset((void*)(VMM_ZERO_WINDOW + (7 * PMM_PAGE_SIZE)), 0, PMM_PAGE_SIZE);

    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Allocated page at unaligned address %x\n", addr);
        return 0;
    }

    return addr;
}

/**
 * @brief Zeroes and then frees a single physical page.
 * @param addr The physical address of the page to free.
 */
void pmm_zfree_page(phys_addr_t addr) {
    if (!addr) {
        serial_printf("PMM: Error: Attempt to free null page\n");
        return;
    }

    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Attempt to free unaligned page at address %x\n", addr);
        return;
    }

    vmm_prepare_zero_window(addr, 8);
    memset((void*)(VMM_ZERO_WINDOW + (8 * PMM_PAGE_SIZE)), 0, PMM_PAGE_SIZE);

    pmm_free_page(addr);
}

/**
 * @brief Allocates a contiguous range of physical pages.
 * @param count The number of pages to allocate.
 * @return The physical address of the first page, or 0 on failure.
 */
phys_addr_t pmm_alloc_pages(size_t count) {
    if (count == 0 || count > pmm_state.max_pages) {
        serial_printf("PMM: Error: Invalid page count %d for allocation\n", count);
        return 0;
    }

    uint32_t* bitmap32 = (uint32_t*)pmm_state.bitmap;
    uint32_t max_blocks = (pmm_state.max_pages / 8) / 4;

    size_t consecutive_found = 0;
    uint32_t start_index = pmm_state.last_checked_index;

    for (uint32_t i = 0; i < max_blocks; i++) {
        uint32_t index = (start_index + i) % max_blocks;
        if (bitmap32[index] == 0xFFFFFFFF) {
            consecutive_found = 0;
        } else {
            for (size_t bit = 0; bit < 32; bit++) {
                if (!(bitmap32[index] & (1 << bit))) {
                    consecutive_found++;
                    if (consecutive_found == count) {
                        uint32_t page_index = (index * 32) + (bit - count + 1);
                        phys_addr_t addr = page_index * PMM_PAGE_SIZE;
                        pmm_lock_pages(addr, count);
                        pmm_state.last_checked_index = index; // start next search from here
                        return addr;
                    }
                } else {
                    consecutive_found = 0;
                }
            }
        }
    }

    uint32_t start_page_for_rest = max_blocks * 32;
    consecutive_found = 0;

    for (uint32_t page_index = start_page_for_rest; page_index < pmm_state.max_pages; page_index++) {
        if (pmm_is_page_free((phys_addr_t)page_index * PMM_PAGE_SIZE)) {
            consecutive_found++;
            if (consecutive_found == count) {
                phys_addr_t addr = (page_index - count + 1) * PMM_PAGE_SIZE;
                pmm_lock_pages(addr, count);
                pmm_state.last_checked_index = page_index / 32; // start next search from here
                return addr;
            }
        } else {
            consecutive_found = 0;
        }
    }

    return 0;
}

/**
 * @brief Frees a contiguous range of physical pages.
 * @param addr The physical address of the first page.
 * @param count The number of pages to free.
 */
void pmm_free_pages(phys_addr_t addr, size_t count) {
    if (!addr) {
        serial_printf("PMM: Error: Attempt to free pages with null starting address\n");
        return;
    }

    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Attempt to free pages with unaligned starting address %x\n", addr);
        return;
    }

    if (count == 0 || count > pmm_state.max_pages) {
        serial_printf("PMM: Error: Invalid page count %d for freeing\n", count);
        return;
    }

    pmm_unlock_pages(addr, count);
}

/**
 * @brief Allocates and zeroes a contiguous range of physical pages.
 * @param count The number of pages to allocate.
 * @return The physical address of the first page, or 0 on failure.
 */
phys_addr_t pmm_zalloc_pages(size_t count) {
    if (count == 0 || count > pmm_state.max_pages) {
        serial_printf("PMM: Error: Invalid page count %d for allocation\n", count);
        return 0;
    }

    phys_addr_t addr = pmm_alloc_pages(count);

    if (!addr) {
        serial_printf("PMM: Error: Failed to allocate pages\n");
        return 0;
    }

    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Allocated pages at unaligned address %x\n", addr);
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        phys_addr_t page_addr = addr + (i * PMM_PAGE_SIZE);
        vmm_prepare_zero_window(page_addr, 9);
        memset((void*)(VMM_ZERO_WINDOW + (9 * PMM_PAGE_SIZE)), 0, PMM_PAGE_SIZE);
    }

    return addr;
}

/**
 * @brief Zeroes and then frees a contiguous range of physical pages.
 * @param addr The physical address of the first page.
 * @param count The number of pages to free.
 */
void pmm_zfree_pages(phys_addr_t addr, size_t count) {
    if (count == 0 || count > pmm_state.max_pages) {
        serial_printf("PMM: Error: Invalid page count %d for freeing\n", count);
        return;
    }

    if (!addr) {
        serial_printf("PMM: Error: Attempt to free pages with null starting address\n");
        return;
    }

    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Attempt to free pages with unaligned starting address %x\n", addr);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        phys_addr_t page_addr = addr + (i * PMM_PAGE_SIZE);
        vmm_prepare_zero_window(page_addr, 10);
        memset((void*)(VMM_ZERO_WINDOW + (10 * PMM_PAGE_SIZE)), 0, PMM_PAGE_SIZE);
    }

    pmm_free_pages(addr, count);
}

/**
 * @brief Marks a range of pages as used in the bitmap.
 * @param addr The starting physical address.
 * @param count The number of pages to lock.
 */
void pmm_lock_pages(phys_addr_t addr, size_t count) {
    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Attempt to lock pages with unaligned starting address %x\n", addr);
        return;
    }

    if (count == 0 || count > pmm_state.max_pages) {
        serial_printf("PMM: Error: Invalid page count %d for locking\n", count);
        return;
    }

    uint32_t* bitmap32 = (uint32_t*)pmm_state.bitmap;
    size_t start_page = addr / PMM_PAGE_SIZE;
    size_t end_page = start_page + count;

    for (size_t i = start_page; i < end_page; ) {
        if (i % 32 == 0 && (end_page - i) >= 32) {
            bitmap32[i / 32] |= 0xFFFFFFFF;
            i += 32;
        } else {
            // Use the byte-based bitmap for unaligned or small ranges
            pmm_state.bitmap[i / 8] |= (1 << (i % 8)); // lock 1 page
            i++; // skip 1 page
        }
    }

    pmm_state.used_pages += count;
}

/**
 * @brief Marks a range of pages as free in the bitmap.
 * @param addr The starting physical address.
 * @param count The number of pages to unlock.
 */
void pmm_unlock_pages(phys_addr_t addr, size_t count) {
    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Attempt to unlock pages with unaligned starting address %x\n", addr);
        return;
    }

    if (count == 0 || count > pmm_state.max_pages) {
        serial_printf("PMM: Error: Invalid page count %d for unlocking\n", count);
        return;
    }

    uint32_t* bitmap32 = (uint32_t*)pmm_state.bitmap;
    size_t start_page = addr / PMM_PAGE_SIZE;
    size_t end_page = start_page + count;

    for (size_t i = start_page; i < end_page; ) {
        if (i % 32 == 0 && (end_page - i) >= 32) {
            bitmap32[i / 32] &= 0x00000000;
            i += 32;
        } else {
            pmm_state.bitmap[i / 8] &= ~(1 << (i % 8)); // unlock 1 page
            i++; // skip 1 page
        }
    }

    pmm_state.used_pages -= count;
}

/**
 * @brief Checks if a specific physical page is free.
 * @param addr The physical address of the page.
 * @return true if the page is free, false otherwise.
 */
bool pmm_is_page_free(phys_addr_t addr) {
    if (!PMM_IS_PAGE_ALIGNED(addr)) {
        serial_printf("PMM: Error: Attempt to check unaligned page at address %x\n", addr);
        return false;
    }

    if (addr >= pmm_state.max_pages * PMM_PAGE_SIZE) {
        serial_printf("PMM: Error: Attempt to check page at out-of-bounds address %x\n", addr);
        return false;
    }

    return !(pmm_state.bitmap[PMM_BITMAP_INDEX(addr)] & (1 << PMM_BITMAP_OFFSET(addr)));
}

/**
 * @brief Returns the total amount of free physical memory in bytes.
 */
uint64_t pmm_get_free_memory(void) {
    return (pmm_state.max_pages - pmm_state.used_pages) * PMM_PAGE_SIZE;
}

/**
 * @brief Returns the total amount of used physical memory in bytes.
 */
uint64_t pmm_get_used_memory(void) {
    return pmm_state.used_pages * PMM_PAGE_SIZE;
}

/**
 * @brief Returns the total amount of physical memory in bytes.
 */
uint64_t pmm_get_total_memory(void) {
    return pmm_state.max_pages * PMM_PAGE_SIZE;
}

/**
 * @brief Returns a pointer to the internal PMM state structure.
 * 
 * Used by the VMM to locate the bitmap for mapping.
 * @return Pointer to pmm_state.
 */
pmm_state_t* pmm_get_state(void) {
    return &pmm_state;
}
