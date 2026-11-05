/**
 * @file smp_trampoline.h
 * @brief Symbol definitions for smp_trampoline.asm.
 * @author friedrichOsDev
 */

#include <pch.h>

extern uint8_t smp_trampoline_start[];
extern uint8_t smp_trampoline_end[];
extern uint64_t smp_trampoline_pml4;
extern uint64_t smp_trampoline_stack;
extern uint64_t smp_trampoline_entry;
