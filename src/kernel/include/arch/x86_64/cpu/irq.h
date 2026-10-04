/**
 * @file irq.h
 * @brief Schnittstelle zur Initialisierung von Hardware-IRQs.
 * @author friedrichOsDev
 */

#pragma once

/**
 * @brief Deaktiviert den Legacy-PIC und registriert die ersten 16 Hardware-IRQ-Vektoren in der IDT.
 */
void irq_init();