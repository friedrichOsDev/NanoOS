#pragma once

#define MAX_CPUS 64
#define ASM_CODE __asm__ __volatile__
#define KERNEL_START (uintptr_t) kernel_start
#define KERNEL_END (uintptr_t) kernel_end
#define KERNEL_START_PHYS (uintptr_t) kernel_start_phys
#define KERNEL_END_PHYS (uintptr_t) kernel_end_phys
#define ISR_COUNT 32
#define IRQ_COUNT 48
#define MMAP_MAX_ENTRIES 1024
#define MAX_MODULES 16
