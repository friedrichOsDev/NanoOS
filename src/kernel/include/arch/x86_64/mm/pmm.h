/**
 * @file pmm.h
 * @brief Physische Speicherverwaltung (Physical Memory Management - PMM).
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <stdint.h>

/**
 * @struct pmm_state_t
 * @brief Zustand und Metriken der physischen Speicherverwaltung (Bitmap-Verwaltung).
 */
typedef struct {
    uint8_t *bitmap;      /**< Zeiger auf die Bitmap, die den Belegungsstatus jeder Seite speichert. */
    uint64_t bitmap_size; /**< Größe der Bitmap in Bytes. */
    uint64_t total_pages; /**< Gesamtzahl der verwalteten physischen Seiten (4 KiB). */
    uint64_t used_pages;  /**< Anzahl der aktuell belegten Seiten. */
    uint64_t free_pages;  /**< Anzahl der aktuell freien Seiten. */
} pmm_state_t;

/** @brief Globale Instanz des PMM-Status. */
extern pmm_state_t pmm_state;

/**
 * @brief Initialisiert den physischen Speicherverwalter.
 *
 * Parst die Memory Map vom Bootloader, baut die Bitmap auf und sperrt reservierte
 * Bereiche wie Kernel, Bitmap, Framebuffer und den Speicher unter 1 MiB.
 */
void pmm_init();

/**
 * @brief Allokiert eine freie physische 4-KiB-Seite.
 *
 * Sucht über Bitmuster-Scans (QWORD) nach einer freien Seite und markiert diese als belegt.
 *
 * @return Physische Startadresse der allokierten Seite oder `0` im Fehlerfall/Speichermangel.
 */
phys_addr_t pmm_page_alloc();

/**
 * @brief Gibt eine zuvor allokierte physische Seite frei.
 *
 * Markiert die entsprechende Seite in der Bitmap als frei.
 *
 * @param addr Physische Adresse der freizugebenden Seite (muss an PAGE_SIZE ausgerichtet sein).
 */
void pmm_page_free(phys_addr_t addr);