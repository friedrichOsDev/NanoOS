#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <serial.h>

#define MSR_IA32_MTRRCAP 0xFE
#define MSR_IA32_MTRR_DEF_TYPE 0x2FF
#define MSR_IA32_MTRR_PHYSBASE0 0x200
#define MSR_IA32_MTRR_PHYSMASK0 0x201

void cpu_enable_write_combining(uint32_t base, uint32_t size);

static inline void cpu_halt() {
    serial_puts("cpu_halt: halting cpu\n");
    __asm__ __volatile__ ("hlt");
}

static inline void cpu_enable_interrupts() {
    __asm__ __volatile__ ("sti");
    serial_puts("cpu_enable_interrupts: interrupts enabled\n");
}

static inline void cpu_disable_interrupts() {
    __asm__ __volatile__ ("cli");
    serial_puts("cpu_disable_interrupts: interrupts disabled\n");
}

static inline void cpu_get_msr(uint32_t msr, uint32_t* low, uint32_t* high) {
    __asm__ __volatile__ (
        "rdmsr"
        : "=a" (*low), "=d" (*high)
        : "c" (msr)
    );
}

static inline void cpu_set_msr(uint32_t msr, uint32_t low, uint32_t high) {
    __asm__ __volatile__ (
        "wrmsr"
        :
        : "c" (msr), "a" (low), "d" (high)
    );
}

static inline uint32_t cpu_get_cr0() {
    uint32_t cr0;
    __asm__ __volatile__ (
        "mov %%cr0, %0"
        : "=r" (cr0)
    );
    return cr0;
}

static inline void cpu_set_cr0(uint32_t cr0) {
    __asm__ __volatile__ (
        "mov %0, %%cr0"
        :
        : "r" (cr0)
    );
}

static inline void cpu_wbinvd() {
    __asm__ __volatile__ ("wbinvd");
}

#endif // CPU_H