#pragma once

#include <pch.h>

// serial port on x86_64
void serial_init();
void serial_putc(char_ascii c);
void serial_puts(const char_ascii *str);
void serial_printf(const char_ascii *format, ...);

// gdt on x86_64
void gdt_init();
void gdt_init_core(size_t cpu_id, uintptr_t kernel_stack);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void gdt_set_tss_gate(int num, uintptr_t base, uint32_t limit);
void gdt_set_tss_kernel_stack(size_t cpu_id, uint64_t stack_top);
