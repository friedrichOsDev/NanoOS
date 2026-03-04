/**
 * @file convert.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

int uint_to_str(uint32_t value, char* buffer, int base);
uint8_t bcd_to_dezimal(uint8_t bcd);
