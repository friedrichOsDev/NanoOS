/**
 * @file gdt_sym.h
 * @brief Symbol definitions for gdt_flush.asm.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

extern void gdt_flush(uint64_t gdt_ptr_phys);
extern void tss_load(uint16_t selector);
