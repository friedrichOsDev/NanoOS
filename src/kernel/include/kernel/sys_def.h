#pragma once

#define MAX_CPUS 64
#define ASM_CODE __asm__ __volatile__
#define KERNEL_START (uintptr_t) kernel_start
#define KERNEL_END (uintptr_t) kernel_end
#define KERNEL_START_PHYS (uintptr_t) kernel_start_phys
#define KERNEL_END_PHYS (uintptr_t) kernel_end_phys
