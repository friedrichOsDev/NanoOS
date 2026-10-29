/**
 * @file fpu.c
 * @brief Implementierung der FPU- und SSE-Initialisierung.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <kernel/panic.h>
#include <lib/util/mem.h>

void fpu_init() {
    uint64_t cr0, cr4;

    ASM_CODE("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1U << 2); // Clear EM (Emulation)
    cr0 |= (1U << 1);  // Set MP (Monitor Coprocessor)
    cr0 &= ~(1U << 3); // Clear TS (Task Switched)
    ASM_CODE("mov %0, %%cr0" ::"r"(cr0));

    ASM_CODE("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1U << 9);  // Enable FXSAVE/FXRSTOR & SSE
    cr4 |= (1U << 10); // Enable Unmasked SSE Exceptions
    ASM_CODE("mov %0, %%cr4" ::"r"(cr4));

    ASM_CODE("fninit");
}

void fpu_state_init(void *fpu_buf) {
    if (!fpu_buf)
        return;

    if ((uintptr_t)fpu_buf % 16 != 0) {
        panic();
    }

    memset(fpu_buf, 0, 512);

    uint32_t *mxcsr_ptr = (uint32_t *)((uint8_t *)fpu_buf + 24);
    uint32_t *mxcsr_mask_ptr = (uint32_t *)((uint8_t *)fpu_buf + 28);

    *mxcsr_ptr = 0x1F80;      // Default MXCSR
    *mxcsr_mask_ptr = 0xFFFF; // Default MXCSR Mask
}
