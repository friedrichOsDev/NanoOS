/*
 * @file pmm.c
 * @brief Physical Memory Manager (PMM)
 * @author friedrichOsDev
 */

#include <pmm.h>
#include <serial.h>
#include <kernel.h>
#include <panic.h>
#include <string.h>

static pmm_state_t pmm_state;

/*
 * Initialize the Physical Memory Manager
 */
void pmm_init(void) {
    phys_addr_t max_addr = 0;

    // get the highest physical address from the memory map
    for (uint32_t i = 0; i < kernel_mmap.entry_count; i++) {
        if (kernel_mmap.entries[i].type == MMAP_USABLE) {
            uint64_t full_end = (uint64_t)kernel_mmap.entries[i].base_addr + kernel_mmap.entries[i].length;
            if (full_end > PMM_MAX_PHYS_ADDR) full_end = PMM_MAX_PHYS_ADDR;
            
            phys_addr_t end_addr = (phys_addr_t)full_end;
            if (end_addr > max_addr) max_addr = end_addr;
        }
    }

    pmm_state.max_pages = max_addr / PAGE_SIZE;
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
            if (candidate == 0) candidate += PAGE_SIZE;

            // Check for kernel overlap
            phys_addr_t kernel_start = ALIGN_DOWN(KERNEL_START_PHYS);
            phys_addr_t kernel_end = ALIGN_UP(KERNEL_END_PHYS);

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
            phys_addr_t start = ALIGN_UP(kernel_mmap.entries[i].base_addr);
            phys_addr_t end = ALIGN_DOWN(kernel_mmap.entries[i].base_addr + kernel_mmap.entries[i].length);
            pmm_unlock_pages(start, (end - start) / PAGE_SIZE);
        }
    }

    // lock kernel && bitmap && framebuffer && multiboot structure && (later acpi __TODO__)
    phys_addr_t kernel_start_aligned = ALIGN_DOWN(KERNEL_START_PHYS);
    phys_addr_t kernel_end_aligned = ALIGN_UP(KERNEL_END_PHYS);
    pmm_lock_pages(kernel_start_aligned, (kernel_end_aligned - kernel_start_aligned) / PAGE_SIZE);

    phys_addr_t bitmap_start_aligned = ALIGN_DOWN((phys_addr_t)pmm_state.bitmap);
    phys_addr_t bitmap_end_aligned = ALIGN_UP((phys_addr_t)pmm_state.bitmap + bitmap_size);
    pmm_lock_pages(bitmap_start_aligned, (bitmap_end_aligned - bitmap_start_aligned) / PAGE_SIZE);

    if (kernel_fb_info.fb_addr) {
        phys_addr_t fb_start_aligned = ALIGN_DOWN((uintptr_t)kernel_fb_info.fb_addr);
        phys_addr_t fb_end_aligned = ALIGN_UP((uintptr_t)kernel_fb_info.fb_addr + (kernel_fb_info.fb_height * kernel_fb_info.fb_pitch));
        pmm_lock_pages(fb_start_aligned, (fb_end_aligned - fb_start_aligned) / PAGE_SIZE);
    }

    phys_addr_t multiboot_start_aligned = ALIGN_DOWN((phys_addr_t)kernel_multiboot_info);
    phys_addr_t multiboot_end_aligned = ALIGN_UP((phys_addr_t)kernel_multiboot_info + kernel_multiboot_info->total_size);
    pmm_lock_pages(multiboot_start_aligned, (multiboot_end_aligned - multiboot_start_aligned) / PAGE_SIZE);

    serial_printf("PMM: Initialized with max address %x, total pages: %d\n", max_addr, pmm_state.max_pages);
    serial_printf("PMM: Free memory: %d KB, Used memory: %d KB\n", (uint32_t)(pmm_get_free_memory() / 1024), (uint32_t)(pmm_get_used_memory() / 1024));
}

/*
 * Allocate a single physical page
 * @return The physical address of the allocated page, or 0 if none available
 */
phys_addr_t pmm_alloc_page() {
    return pmm_alloc_pages(1);
}

/*
 * Free a single physical page
 * @param addr The physical address of the page to free
 */
void pmm_free_page(phys_addr_t addr) {
    pmm_free_pages(addr, 1);
}

/*
 * Allocate a single physical page and zero it
 * @return The physical address of the allocated page, or 0 if none available
 */
phys_addr_t pmm_zalloc_page() {
    phys_addr_t addr = pmm_alloc_page();
    if (addr) memset((void*)addr, 0, PAGE_SIZE);
    return addr;
}

/*
 * Free a single physical page and zero it
 * @param addr The physical address of the page to free
 */
void pmm_zfree_page(phys_addr_t addr) {
    if (addr) memset((void*)addr, 0, PAGE_SIZE);
    pmm_free_page(addr);
}

/*
 * Allocate a contiguous block of physical pages
 * @param count The number of pages to allocate
 * @return The physical address of the first page, or 0 if not found
 */
phys_addr_t pmm_alloc_pages(size_t count) {
    if (count == 0 || count > pmm_state.max_pages) return 0;

    uint32_t* bitmap32 = (uint32_t*)pmm_state.bitmap;
    uint32_t max_blocks = (pmm_state.max_pages / 8) / 4;

    size_t consecutive_found = 0;
    uint32_t start_index = pmm_state.last_checked_index;

    for (uint32_t i = 0; i < max_blocks; i++) {
        uint32_t index = (start_index + i) % max_blocks;
        if (bitmap32[index] == 0xFFFFFFFF) {
            consecutive_found = 0; // all pages in this block are used
        } else {
            for (size_t bit = 0; bit < 32; bit++) {
                if (!(bitmap32[index] & (1 << bit))) {
                    consecutive_found++;
                    if (consecutive_found == count) {
                        uint32_t page_index = (index * 32) + (bit - count + 1);
                        phys_addr_t addr = page_index * PAGE_SIZE;
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

    // if max_blocks % 32 != 0
    uint32_t start_page_for_rest = max_blocks * 32;
    consecutive_found = 0;

    for (uint32_t page_index = start_page_for_rest; page_index < pmm_state.max_pages; page_index++) {
        if (pmm_is_page_free(page_index * PAGE_SIZE)) {
            consecutive_found++;
            if (consecutive_found == count) {
                phys_addr_t addr = (page_index - count + 1) * PAGE_SIZE;
                pmm_lock_pages(addr, count);
                pmm_state.last_checked_index = page_index / 32; // start next search from here
                return addr;
            }
        } else {
            consecutive_found = 0;
        }
    }

    return 0; // no suitable block found
}

/*
 * Free a contiguous block of physical pages
 * @param addr The physical address of the first page
 * @param count The number of pages to free
 */
void pmm_free_pages(phys_addr_t addr, size_t count) {
    if (!IS_PAGE_ALIGNED(addr)) return;
    for (size_t i = 0; i < count; i++) {
        pmm_unlock_page(addr + (i * PAGE_SIZE));
    }
}

/*
 * Allocate a contiguous block of physical pages and zero them
 * @param count The number of pages to allocate
 * @return The physical address of the first page, or 0 if not found
 */
phys_addr_t pmm_zalloc_pages(size_t count) {
    phys_addr_t addr = pmm_alloc_pages(count);
    if (addr) memset((void*)addr, 0, count * PAGE_SIZE);
    return addr;
}

/*
 * Free a contiguous block of physical pages and zero them
 * @param addr The physical address of the first page
 * @param count The number of pages to free
 */
void pmm_zfree_pages(phys_addr_t addr, size_t count) {
    if (addr && IS_PAGE_ALIGNED(addr)) {
        memset((void*)addr, 0, count * PAGE_SIZE);
        pmm_free_pages(addr, count);
    }
}

/*
 * Lock a specific physical page
 * @param addr The physical address of the page to lock
 */
void pmm_lock_page(phys_addr_t addr) {
    if (addr >= pmm_state.max_pages * PAGE_SIZE) return;
    if (!IS_PAGE_ALIGNED(addr)) return;
    if (pmm_is_page_free(addr)) {
        pmm_state.bitmap[BITMAP_INDEX(addr)] |= (1 << BITMAP_OFFSET(addr));
        pmm_state.used_pages++;
    }
}

/*
 * Unlock a specific physical page
 * @param addr The physical address of the page to unlock
 */
void pmm_unlock_page(phys_addr_t addr) {
    if (addr >= pmm_state.max_pages * PAGE_SIZE) return;
    if (!IS_PAGE_ALIGNED(addr)) return;
    if (!pmm_is_page_free(addr)) {
        pmm_state.bitmap[BITMAP_INDEX(addr)] &= ~(1 << BITMAP_OFFSET(addr));
        pmm_state.used_pages--;
    }
}

/*
 * Lock a range of physical pages
 * @param addr The physical address of the first page
 * @param count The number of pages to lock
 */
void pmm_lock_pages(phys_addr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        pmm_lock_page(addr + (i * PAGE_SIZE));
    }
}

/*
 * Unlock a range of physical pages
 * @param addr The physical address of the first page
 * @param count The number of pages to unlock
 */
void pmm_unlock_pages(phys_addr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        pmm_unlock_page(addr + (i * PAGE_SIZE));
    }
}

/*
 * Check if a specific physical page is free
 * @param addr The physical address of the page to check
 * @return True if the page is free, false otherwise
 */
bool pmm_is_page_free(phys_addr_t addr) {
    if (addr >= pmm_state.max_pages * PAGE_SIZE) return false;
    return !(pmm_state.bitmap[BITMAP_INDEX(addr)] & (1 << BITMAP_OFFSET(addr)));
}

/*
 * Get the amount of free physical memory
 * @return The amount of free memory in bytes
 */
uint64_t pmm_get_free_memory(void) {
    return (pmm_state.max_pages - pmm_state.used_pages) * PAGE_SIZE;
}

/*
 * Get the amount of used physical memory
 * @return The amount of used memory in bytes
 */
uint64_t pmm_get_used_memory(void) {
    return pmm_state.used_pages * PAGE_SIZE;
}

/*
 * Get the total amount of physical memory
 * @return The total amount of memory in bytes
 */
uint64_t pmm_get_total_memory(void) {
    return pmm_state.max_pages * PAGE_SIZE;
}
