/**
 * @file fpu.c
 * @brief FPU and SSE management implementation
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/fpu.h>
#include <core/panic.h>
#include <lib/string.h>

void cpu_fpu_init(void) {
    uint64_t cr0, cr4;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Clear EM (Emulation)
    cr0 |= (1 << 1);  // Set MP (Monitor Coprocessor)
    cr0 &= ~(1 << 3); // Clear TS (Task Switched)
    asm volatile("mov %0, %%cr0" ::"r"(cr0));

    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);  // Enable FXSAVE/FXRSTOR & SSE
    cr4 |= (1 << 10); // Enable Unmasked SSE Exceptions
    asm volatile("mov %0, %%cr4" ::"r"(cr4));

    asm volatile("fninit");
}

void fpu_state_init(void *fpu_buf) {
    if (!fpu_buf)
        return;

    if ((uintptr_t)fpu_buf % 16 != 0) {
        panic("FPU: fpu_state buffer not 16-byte aligned (fxsave64/fxrstor64)", (uintptr_t)fpu_buf % 16);
    }

    memset(fpu_buf, 0, 512);

    uint32_t *mxcsr_ptr = (uint32_t *)((uint8_t *)fpu_buf + 24);
    uint32_t *mxcsr_mask_ptr = (uint32_t *)((uint8_t *)fpu_buf + 28);

    *mxcsr_ptr = 0x1F80;      // Default MXCSR
    *mxcsr_mask_ptr = 0xFFFF; // Default MXCSR Mask
}