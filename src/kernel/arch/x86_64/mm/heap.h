/**
 * @file heap.h
 * @brief Kernel-Heap-Allocator.
 * @author friedrichOsDev
 */

#pragma once

#include "../mm/mem_def.h"
#include <pch.h>

/**
 * @brief Initialisiert den Kernel-Heap-Allocator.
 *
 * Allokiert die ersten 4 physischen Seiten, mappt sie in den Kernel-Adressraum
 * und richtet den initialen freien Heap-Block ein.
 */

/**
 * @brief Erweitert den Heap dynamisch um mindestens die angeforderte Größe.
 *
 * Allokiert weitere physische Seiten, mappt diese lückenlos an das aktuelle Ende
 * des Heaps und verschmilzt sie gegebenenfalls mit dem vorherigen freien Block.
 *
 * @param size Benötigte Mindestgröße für die Erweiterung in Bytes.
 * @return Zeiger auf den neuen bzw. erweiterten freien Heap-Block (`heap_node_t*`).
 */
heap_node_t *
heap_extend(size_t size);
