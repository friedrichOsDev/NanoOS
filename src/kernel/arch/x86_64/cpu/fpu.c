/**
 * @file fpu.c
 * @brief FPU and SSE management implementation
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/fpu.h>
#include <lib/string.h>

void cpu_fpu_init(void) {
    uint64_t cr0, cr4;

    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~((1ULL << 2) | (1ULL << 3)); // Clear EM (bit 2) and TS (bit 3)
    cr0 |= (1ULL << 1);                  // Set MP (bit 1)
    __asm__ __volatile__("mov %0, %%cr0" ::"r"(cr0));

    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9) | (1ULL << 10); // Set OSFXSR (bit 9) and OSXMMEXCPT (bit 10)
    __asm__ __volatile__("mov %0, %%cr4" ::"r"(cr4));

    __asm__ __volatile__("fninit");

    uint32_t mxcsr = 0x1F80; // All exception masks set, round-to-nearest
    __asm__ __volatile__("ldmxcsr %0" ::"m"(mxcsr));
}

void fpu_state_init(void *fpu_buf) {
    if (!fpu_buf)
        return;

    memset(fpu_buf, 0, 512);

    uint32_t *mxcsr_ptr = (uint32_t *)((uint8_t *)fpu_buf + 24);
    uint32_t *mxcsr_mask_ptr = (uint32_t *)((uint8_t *)fpu_buf + 28);

    *mxcsr_ptr = 0x1F80;      // Default MXCSR
    *mxcsr_mask_ptr = 0xFFFF; // Default MXCSR Mask
}