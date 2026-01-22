/*
 * @file paging.h
 * @brief Header file for paging implementation
 * @author friedrichOsDev
 */

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGING_PAGE_SIZE 4096
#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024

#define PAGE_DIRECTORY_MASK 0xFFFFF000

#define PAGING_CACHE_DISABLED  0b00010000
#define PAGING_WRITE_THROUGH   0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITABLE     0b00000010
#define PAGING_IS_PRESENT      0b00000001

void paging_map(uint32_t* directory, void* virtual_address, uint32_t value);
uint32_t* paging_get_current_directory(void);
void paging_init(void);

#endif // PAGING_H