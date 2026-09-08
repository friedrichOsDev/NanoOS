/**
 * @file fpu.h
 * @brief FPU and SSE management header
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

void cpu_fpu_init(void);
void fpu_state_init(void *fpu_buf);