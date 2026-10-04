/**
 * @file vmm.h
 * @brief Virtuelle Speicherverwaltung (Virtual Memory Management - VMM).
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <stddef.h>

/** @brief Virtuelle Adresse der globalen Kernel-PML4-Seitentabelle. */
extern virt_addr_t kernel_pml4;

/**
 * @brief Initialisiert das VMM-Subsystem.
 *
 * Allokiert die Kernel-PML4-Tabelle, richtet Direct-Mapping für den physischen RAM,
 * Higher-Half-Mappings für den Kernel/Bitmap sowie Identity-Mapping ein und lädt CR3.
 */
void vmm_init();

/**
 * @brief Mappt einen physischen MMIO-Bereich in den virtuellen MMIO-Speicherbereich.
 *
 * @param pml4 Zeiger auf die PML4-Seitentabelle.
 * @param paddr Physische Startadresse des MMIO-Bereichs.
 * @param size Größe des zu mappenden Bereichs in Bytes.
 * @return Virtuelle Startadresse des gemappten MMIO-Bereichs oder `0` bei Fehler.
 */
virt_addr_t vmm_map_mmio(page_table_t *pml4, phys_addr_t paddr, size_t size);

/**
 * @brief Mappt eine einzelne physische Seite auf eine virtuelle Seite.
 *
 * Traversiert/allokiert nötige Paging-Tabellenstufen (PDPT, PD, PT) und invalidiert den TLB.
 *
 * @param pml4 Zeiger auf die PML4-Seitentabelle.
 * @param vaddr Virtuelle Zieladresse (muss page-aligned sein).
 * @param paddr Physische Quelladresse (muss page-aligned sein).
 * @param flags Zugriffsberechtigungen/Flags für den Tabelleneintrag (z. B. `PTE_WRITABLE`).
 */
void vmm_map_page(page_table_t *pml4, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);

/**
 * @brief Hebt das Mapping einer virtuellen Seite auf.
 *
 * Löscht den PTE, invalidiert den TLB-Eintrag und gibt leere Untertabellen im Paging-Baum frei.
 *
 * @param pml4 Zeiger auf die PML4-Seitentabelle.
 * @param vaddr Virtuelle Adresse der freizugebenden Seite (muss page-aligned sein).
 */
void vmm_unmap_page(page_table_t *pml4, virt_addr_t vaddr);