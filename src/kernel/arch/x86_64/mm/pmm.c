/**
 * @file pmm.c
 * @brief Physical Memory Management
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/pmm.h>
#include <core/init.h>
#include <core/panic.h>
#include <lib/string.h>
#include <core/sync.h>

pmm_state_t pmm_state;

static spinlock_t pmm_lock = SPINLOCK_INIT;

/**
 * Locks a physical Page in the pmm bitmap
 * @param start The start Page to lock
 * @param count The count of Pages to lock
 */
static void lock_pages(phys_addr_t start, uint64_t count) {
    if (start > pmm_state.total_pages * PAGE_SIZE || !IS_PAGE_ALIGNED(start)) {
        panic("pmm bad start", start);
    }
    if (count > pmm_state.total_pages || count == 0) {
        panic("pmm bad count", count);
    }

    uint64_t start_page = start / PAGE_SIZE;
    if (start_page + count > pmm_state.total_pages) {
        panic("pmm bad start_page", start_page);
    }

    for (uint64_t i = 0; i < count; i++) {
        uint64_t page = start_page + i;
        uint64_t byte_idx = page / 8;
        uint8_t bit_idx = page % 8;

        if ((pmm_state.bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            pmm_state.bitmap[byte_idx] |= (1 << bit_idx);
            pmm_state.used_pages++;
            if (pmm_state.free_pages > 0)
                pmm_state.free_pages--;
        }
    }
}

/**
 * Unlocks a physical Page in the pmm bitmap
 * @param start The start Page to unlock
 * @param count The count of Pages to unlock
 */
static void unlock_pages(phys_addr_t start, uint64_t count) {
    if (start > pmm_state.total_pages * PAGE_SIZE || !IS_PAGE_ALIGNED(start)) {
        panic("pmm unlock bad start", start);
    }
    if (count > pmm_state.total_pages || count == 0) {
        panic("pmm unlock bad count", count);
    }

    uint64_t start_page = start / PAGE_SIZE;
    if (start_page + count > pmm_state.total_pages) {
        panic("pmm unlock bad start_page", start_page);
    }

    for (uint64_t i = 0; i < count; i++) {
        uint64_t page = start_page + i;
        uint64_t byte_idx = page / 8;
        uint8_t bit_idx = page % 8;

        if ((pmm_state.bitmap[byte_idx] & (1 << bit_idx)) != 0) {
            pmm_state.bitmap[byte_idx] &= ~(1 << bit_idx);
            if (pmm_state.used_pages > 0)
                pmm_state.used_pages--;
            pmm_state.free_pages++;
        }
    }
}

/**
 * Parses the Memory Map to get Infos for the pmm_state structure
 */
static void mmap_parse() {
    uint64_t max_usable_addr = 0;

    for (uint64_t i = 0; i < kernel_mmap.entry_count; i++) {
        if (kernel_mmap.entries[i].type == MMAP_USABLE) {
            uint64_t region_end = kernel_mmap.entries[i].base_addr +
                                  kernel_mmap.entries[i].length;
            if (region_end > max_usable_addr) {
                max_usable_addr = region_end;
            }
        }
    }

    uint64_t total_pages = max_usable_addr / PAGE_SIZE;
    uint64_t bitmap_size = total_pages / 8;
    serial_printf(COM1, "PMM: max_addr=%llx, bitmap_size=%llx\n",
                  max_usable_addr, bitmap_size);

    pmm_state.bitmap = (uint8_t *)KERNEL_END_PHYS;
    pmm_state.bitmap_size = bitmap_size;
    pmm_state.total_pages = total_pages;
    pmm_state.used_pages = total_pages;
    pmm_state.free_pages = 0;

    memset(pmm_state.bitmap, 0xFF, pmm_state.bitmap_size);

    for (uint64_t i = 0; i < kernel_mmap.entry_count; i++) {
        if (kernel_mmap.entries[i].type == MMAP_USABLE) {
            mmap_entry_t entry = kernel_mmap.entries[i];

            uint64_t aligned_base = ALIGN_UP(entry.base_addr);
            uint64_t aligned_len = ALIGN_DOWN(entry.length);

            if (aligned_len >= PAGE_SIZE) {
                unlock_pages(aligned_base, aligned_len / PAGE_SIZE);
            }
        }
    }
}

/**
 * Initializes the PMM
 */
void pmm_init() {
    mmap_parse();

    // lock kernel + bitmap
    uint64_t kernel_and_bitmap_size =
        KERNEL_END_PHYS + pmm_state.bitmap_size - KERNEL_START_PHYS;
    lock_pages(KERNEL_START_PHYS, ALIGN_UP(kernel_and_bitmap_size) / PAGE_SIZE);

    // lock framebuffer
    if (kernel_fb_info.fb_addr) {
        uint64_t fb_start_aligned = ALIGN_DOWN(kernel_fb_info.fb_addr);
        uint64_t fb_end_aligned =
            ALIGN_UP(kernel_fb_info.fb_addr +
                     (kernel_fb_info.fb_height * kernel_fb_info.fb_pitch));
        lock_pages(fb_start_aligned,
                   (fb_end_aligned - fb_start_aligned) / PAGE_SIZE);
    }

    // lock below 1M
    lock_pages(0, 256);

    serial_printf(COM1, "PMM: used_pages=%llx, free_pages=%llx\n",
                  pmm_state.used_pages, pmm_state.free_pages);
}

/**
 * Allocates a physical Page and locks it
 * @return Returns the physical address of the allocated Page or 0 on error
 */
phys_addr_t pmm_page_alloc() {
    uint64_t flags = spinlock_acquire_irqsave(&pmm_lock);
    if (pmm_state.free_pages == 0) {
        panic("pmm out of memory", 0);
    }

    uint64_t *bitmap64 = (uint64_t *)pmm_state.bitmap;
    size_t qword_count = pmm_state.bitmap_size / 8;

    for (size_t i = 0; i < qword_count; i++) {
        if (bitmap64[i] != 0xFFFFFFFFFFFFFFFFULL) {
            uint64_t inverted = ~bitmap64[i];
            int bit_idx = __builtin_ctzll(inverted);

            uint64_t page_idx = (i * 64) + bit_idx;

            if (page_idx >= pmm_state.total_pages) {
                spinlock_release_irqrestore(&pmm_lock, flags);
                return 0;
            }

            phys_addr_t alloc_addr = page_idx * PAGE_SIZE;

            lock_pages(alloc_addr, 1);

            spinlock_release_irqrestore(&pmm_lock, flags);
            return alloc_addr;
        }
    }

    spinlock_release_irqrestore(&pmm_lock, flags);
    return 0;
}

/**
 * Frees a physical Page and unlocks it
 * @param addr The address of the physical Page to free
 */
void pmm_page_free(phys_addr_t addr) {
    if (!IS_PAGE_ALIGNED(addr)) {
        panic("pmm free unaligned page", addr);
    }

    uint64_t flags = spinlock_acquire_irqsave(&pmm_lock);
    unlock_pages(addr, 1);
    spinlock_release_irqrestore(&pmm_lock, flags);
}
