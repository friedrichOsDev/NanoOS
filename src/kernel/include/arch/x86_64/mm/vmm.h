/**
 * @file vmm.h
 * @brief Virtual Memory Management (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <stddef.h>

void vmm_init();
virt_addr_t vmm_map_mmio(page_table_t *pml4, phys_addr_t paddr, size_t size);
void vmm_map_page(page_table_t* pml4, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);
void vmm_unmap_page(page_table_t* pml4, virt_addr_t vaddr);
