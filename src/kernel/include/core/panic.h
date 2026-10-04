/**
 * @file panic.h
 * @brief Schnittstelle für den Absturz- und Fehlerbehandlungskernel (Kernel Panic).
 * @details Bietet die zentrale Routine zum Anhalten des Systems bei unbehebbaren
 *          Laufzeitfehlern.
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/**
 * @brief Löst eine Kernel Panic aus und versetzt das System in einen sicheren Halt-Zustand.
 * @details Deaktiviert Interrupts, stoppt alle anderen CPU-Cores via Inter-Processor Interrupt (IPI),
 *          gibt die Fehlermeldung aus und hält die aktuelle CPU dauerhaft an.
 * @param message Beschreibende Fehlermeldung bezüglich der Ursache des Absturzes.
 * @param error_code Spezifischer Fehlercode oder Zusatzinformation (0, falls nicht vorhanden).
 */
void panic(const char *message, uint64_t error_code);