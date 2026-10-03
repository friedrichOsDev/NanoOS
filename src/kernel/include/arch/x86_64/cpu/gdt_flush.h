/**
 * @file gdt_flush.h
 * @brief Deklarationen für das Laden der GDT (Global Descriptor Table) und der TSS (Task State Segment).
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/**
 * @brief Lädt die Global Descriptor Table (GDT) und aktualisiert die Segmentregister.
 *
 * Führt den Assembler-Befehl `lgdt` aus, um den GDTR-Zeiger zu laden, aktualisiert
 * das CS-Register via Far-Return (`retfq`) auf das Kernel-Code-Segment (0x08)
 * und lädt die Daten-Segmentregister (DS, ES, FS, GS, SS) neu mit 0x10.
 *
 * @param gdt_ptr_phys Physische Adresse der GDTR-Struktur (`gdt_ptr_t`).
 */
extern void gdt_flush(uint64_t gdt_ptr_phys);

/**
 * @brief Lädt das Task State Segment (TSS) in das Task-Register (TR).
 *
 * Führt den Assembler-Befehl `ltr` aus, um den Selektor für das TSS zu aktivieren.
 *
 * @param selector GDT-Segmentselektor für das TSS (z. B. 0x28).
 */
extern void tss_load(uint16_t selector);