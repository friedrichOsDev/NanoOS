/**
 * @file irq.h
 * @brief IRQ Setup
 */

#pragma once

/**
 * @brief Disables legacy PIC and registers initial hardware IRQ vectors in IDT
 */
void irq_init();