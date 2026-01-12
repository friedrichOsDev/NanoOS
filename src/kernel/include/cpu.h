#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <serial.h>

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

#endif // CPU_H