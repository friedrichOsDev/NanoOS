/**
 * @file types.h
 * @brief Alle Datentypen für meinen Kernel
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// mm
typedef uint64_t phys_addr_t;
typedef uintptr_t virt_addr_t;

// text
typedef char char_ascii;
typedef uint32_t char_uni;

