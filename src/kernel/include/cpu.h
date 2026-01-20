/*
 * @file cpu.h
 * @brief Header file for CPU-related functions
 * @author friedrichOsDev
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MSR_IA32_MTRRCAP 0xFE
#define MSR_IA32_MTRR_DEF_TYPE 0x2FF
#define MSR_IA32_MTRR_PHYSBASE0 0x200
#define MSR_IA32_MTRR_PHYSMASK0 0x201

void cpu_halt(void);
void cpu_enable_interrupts(void);
void cpu_disable_interrupts(void);
void cpu_get_msr(uint32_t msr, uint32_t* low, uint32_t* high);
void cpu_set_msr(uint32_t msr, uint32_t low, uint32_t high);
uint32_t cpu_get_cr0(void);
void cpu_set_cr0(uint32_t cr0);
void cpu_wbinvd(void);
void cpu_mfence(void);
void cpu_enable_write_combining(uint32_t base, uint32_t size);

#endif // CPU_H