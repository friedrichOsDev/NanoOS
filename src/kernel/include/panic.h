/**
 * @file panic.h
 * @author friedrichOsDev 
 */

#pragma once

#include <stdint.h>

void kernel_panic(const char* error_msg, uint32_t error_code);