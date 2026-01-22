/*
 * @file paging.c
 * @brief Paging implementation
 * @author friedrichOsDev
 */

#include <paging.h>
#include <serial.h>
#include <heap.h>
#include <handler.h>

/*
 * Load the page directory into the CPU's CR3 register
 * @param directory A pointer to the page directory
 */
extern void load_page_directory(uint32_t* directory);

/*
 * Enable paging by setting the appropriate bit in the CR0 register
 * @param void
 */
extern void enable_paging(void);

/*
 * The current page directory
 */
static uint32_t* current_directory = 0;

/*
 * Create a new page directory
 * @param flags The flags to apply to the page directory and tables
 * @return A pointer to the new page directory
 */
static uint32_t* paging_new_directory(uint8_t flags) {
    uint32_t* directory = kzalloc_aligned(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE, PAGING_PAGE_SIZE);
    uint32_t offset = 0;

    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        uint32_t* entry = kzalloc_aligned(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE, PAGING_PAGE_SIZE);
        for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++) {
            entry[j] = (offset + (j * PAGING_PAGE_SIZE)) | flags;
        }
        offset += PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE;
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITABLE;
    }
    return directory;
}

/*
 * Switch to a new page directory
 * @param directory A pointer to the new page directory
 */
static void paging_switch(uint32_t* directory) {
    load_page_directory(directory);
    current_directory = directory;
}

/*
 * Check if an address is aligned to the page size
 * @param address The address to check
 * @return true if the address is aligned, false otherwise
 */
static inline bool paging_is_aligned(void* address) {
    return (uint32_t)address % PAGING_PAGE_SIZE == 0;
}

/*
 * Get the directory and table indexes for a given virtual address
 * @param virtual_address The virtual address to get the indexes for
 * @param directory_index_out A pointer to store the directory index
 * @param table_index_out A pointer to store the table index
 * @return 0 on success, -1 on failure
 */
static int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out) {
    if (!paging_is_aligned(virtual_address)) return -1;

    *directory_index_out = (uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
    *table_index_out = (uint32_t)virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE;
    
    return 0;
}

/*
 * Map a virtual address to a physical address with flags in the page directory
 * @param directory A pointer to the page directory
 * @param virtual_address The virtual address to map
 * @param value The physical address and flags to map to
 */
void paging_map(uint32_t* directory, void* virtual_address, uint32_t value) {
    uint32_t directory_index = 0;
    uint32_t table_index = 0;

    if (paging_get_indexes(virtual_address, &directory_index, &table_index) != 0) return;

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & PAGE_DIRECTORY_MASK);
    table[table_index] = value;

    paging_switch(directory);
    
    serial_puts("paging_set: mapped virtual address ");
    serial_put_hex((uint32_t)virtual_address);
    serial_puts(" to value ");
    serial_put_hex(value);
    serial_puts("\n");
}

/*
 * Page fault handler
 * @param error_code The error code of the page fault
 */
static void page_fault_handler(uint32_t error_code) {
    serial_puts("page_fault_handler: Page fault occurred with error code: ");
    serial_put_int(error_code);
    serial_puts("\n");
    while (1); 
}

/*
 * A function to get the current page directory
 * @param void
 * @return A pointer to the current page directory
 */
uint32_t* paging_get_current_directory(void) {
    return current_directory;
}

/*
 * A function to initialize paging
 * @param void
 */
void paging_init(void) {
    isr_install_handler(14, page_fault_handler);

    uint32_t* kernel_directory = paging_new_directory(PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_directory);
    enable_paging();
}
