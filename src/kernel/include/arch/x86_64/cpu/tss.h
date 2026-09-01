/**
 * @file tss.h
 * @brief 64-bit Task State Segment Setup (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <arch/x86_64/cpu/smp.h>

struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

extern struct tss_entry tss;
extern struct tss_entry tss_cores[MAX_CPUS];

void tss_init_core(size_t cpu_id, uint64_t kernel_stack);