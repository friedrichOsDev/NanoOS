/**
 * @file heap.h
 * @brief Kernel-Heap-Allocator.
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <stddef.h>

/**
 * @brief Initialisiert den Kernel-Heap-Allocator.
 *
 * Allokiert die ersten 4 physischen Seiten, mappt sie in den Kernel-Adressraum
 * und richtet den initialen freien Heap-Block ein.
 */
void heap_init();

/**
 * @brief Erweitert den Heap dynamisch um mindestens die angeforderte Größe.
 *
 * Allokiert weitere physische Seiten, mappt diese lückenlos an das aktuelle Ende
 * des Heaps und verschmilzt sie gegebenenfalls mit dem vorherigen freien Block.
 *
 * @param size Benötigte Mindestgröße für die Erweiterung in Bytes.
 * @return Zeiger auf den neuen bzw. erweiterten freien Heap-Block (`heap_list_t*`).
 */
heap_list_t *heap_extend(size_t size);

/**
 * @brief Allokiert einen Speicherblock aus dem Kernel-Heap (Best-Fit-Strategie).
 *
 * Der angeforderte Speicher wird auf 16 Bytes ausgerichtet. Reicht der vorhandene
 * Platz nicht aus, wird der Heap automatisch erweitert.
 *
 * @param size Größe des anzufordernden Speichers in Bytes.
 * @return Virtuelle Startadresse des zugewiesenen Speicherbereichs (Payload) oder `0` bei Fehler/Größe 0.
 */
virt_addr_t kmalloc(size_t size);

/**
 * @brief Allokiert Speicher aus dem Kernel-Heap und nullt diesen vollständig aus.
 *
 * @param size Größe des anzufordernden Speichers in Bytes.
 * @return Virtuelle Startadresse des genullten Speicherbereichs oder `0` bei Fehler.
 */
virt_addr_t kzalloc(size_t size);

/**
 * @brief Gibt einen zuvor allokierten Heap-Speicherbereich wieder frei.
 *
 * Markiert den Block als frei und verschmilzt ihn automatisch mit angrenzenden
 * freien Blöcken (Coalescing links/rechts), um Fragmentierung zu vermeiden.
 *
 * @param addr Virtuelle Startadresse der Nutzlast (Payload), die freigegeben werden soll.
 */
void kfree(virt_addr_t addr);

/**
 * @brief Gibt den aktuellen Zustand und alle Blöcke der doppelt verketteten Heap-Liste über die serielle Schnittstelle aus.
 */
void heap_dump();