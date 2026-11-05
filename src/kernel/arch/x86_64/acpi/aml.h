/**
 * @file aml.h
 * @brief AML parsing definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define AML_NAME_OP 0x08
#define AML_PACKAGE_OP 0x12
#define AML_BYTE_PREFIX 0x0A
#define AML_WORD_PREFIX 0x0B
#define AML_DWORD_PREFIX 0x0C
#define AML_STRING_PREFIX 0x0D

/** @brief Container holding the parsed S5 sleep type values. */
typedef struct {
    uint16_t slp_typa; /**< SLP_TYPa value for PM1 control block. */
    uint16_t slp_typb; /**< SLP_TYPb value for PM1 control block. */
    bool valid;        /**< Flag indicating whether the S5 values were successfully parsed. */
} s5_sleep_values_t;

bool aml_parse_s5(const void *dsdt_ptr, size_t dsdt_length, s5_sleep_values_t *out_s5);
