#include <paging.h>
#include <serial.h>
#include <heap.h>
#include <handler.h>

extern void load_page_directory(uint32_t* directory);
extern void enable_paging();

static uint32_t* current_directory = 0;

uint32_t* paging_new_directory(uint8_t flags) {
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

void paging_switch(uint32_t* directory) {
    load_page_directory(directory);
    current_directory = directory;
}

bool paging_is_aligned(void* address) {
    return (uint32_t)address % PAGING_PAGE_SIZE == 0;
}

int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out) {
    if (!paging_is_aligned(virtual_address)) return -1;

    *directory_index_out = (uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
    *table_index_out = (uint32_t)virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE;
    
    return 0;
}

void paging_set(uint32_t* directory, void* virtual_address, uint32_t value) {
    uint32_t directory_index = 0;
    uint32_t table_index = 0;

    if (paging_get_indexes(virtual_address, &directory_index, &table_index) != 0) return;

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & PAGE_DIRECTORY_MASK);
    table[table_index] = value;
    
    serial_puts("paging_set: mapped virtual address ");
    serial_put_hex((uint32_t)virtual_address);
    serial_puts(" to value ");
    serial_put_hex(value);
    serial_puts("\n");
}

void page_fault_handler(uint32_t error_code) {
    serial_puts("page_fault_handler: Page fault occurred with error code: ");
    serial_put_int(error_code);
    serial_puts("\n");
    while (1); 
}

void paging_init() {
    isr_install_handler(14, page_fault_handler);
    serial_puts("paging_init: installed page fault handler\n");

    uint32_t* kernel_directory = paging_new_directory(PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_directory);
    enable_paging();
}