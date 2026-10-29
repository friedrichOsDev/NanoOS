/**
 * @file pmm.h
 * @brief Physische Speicherverwaltung (Physical Memory Management - PMM).
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

typedef struct {
    uint8_t *bitmap;      /**< Zeiger auf die Bitmap, die den Belegungsstatus jeder Seite speichert. */
    uint64_t bitmap_size; /**< Größe der Bitmap in Bytes. */
    uint64_t total_pages; /**< Gesamtzahl der verwalteten physischen Seiten (4 KiB). */
    uint64_t used_pages;  /**< Anzahl der aktuell belegten Seiten. */
    uint64_t free_pages;  /**< Anzahl der aktuell freien Seiten. */
} pmm_state_t;

/** @brief Globale Instanz des PMM-Status. */
extern pmm_state_t pmm_state;

