#pragma once

#include <pch.h>

void isr_handler(struct registers *regs);
void irq_handler(struct registers *regs);
void reschedule_ipi_handler(struct registers *regs);
void stop_ipi_handler(struct registers *regs);
void tlb_shootdown_ipi_handler(struct registers *regs);
void lapic_timer_handler(struct registers *regs);
