/**
 * @file fpu.h
 * @brief FPU- und SSE-Konfiguration.
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/**
 * @brief Initialisiert die FPU (Floating Point Unit) und SSE auf dem aktuellen Prozessorkern.
 *
 * Konfiguriert die Steuerregister CR0 und CR4, um Hardware-Fließkommaoperationen und
 * SSE-Befehle zu aktivieren sowie die FXSAVE/FXRSTOR-Instruktionen vorzubereiten.
 */
void cpu_fpu_init();

/**
 * @brief Initialisiert einen 512-Byte-Puffer für den FPU/SSE-Kontext mit Standardwerten.
 *
 * Bereitet einen Puffer für die Speicherung mittels `fxsave64` / `fxrstor64` vor,
 * setzt den MXCSR-Standardwert (0x1F80) und prüft die erforderliche 16-Byte-Ausrichtung.
 *
 * @param fpu_buf Zeiger auf den mindestens 512 Byte großen, 16-Byte-orientierten Speicherbereich.
 */
void fpu_state_init(void *fpu_buf);