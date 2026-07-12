/**
 * @file mm.h
 * @brief Memory Manager
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define PAGE_SIZE 0x1000

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;