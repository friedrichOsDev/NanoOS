/**
 * @file aml.c
 * @brief AML parsing implementation for DSDT objects.
 * @author friedrichOsDev
 */

#include "aml.h"
#include <lib/util/mem.h>

/**
 * @brief Parses the AML package length (PkgLength) according to the ACPI specification.
 * 
 * @param[in] ptr Pointer to the start of the package length encoding.
 * @param[in] end Pointer to the end of the available buffer.
 * @param[out] out_len Pointer to store the decoded package length.
 * @param[out] out_bytes_read Pointer to store the number of bytes consumed by the length encoding.
 * @return true on success, false if a buffer overflow or invalid encoding is detected.
 */
static bool aml_parse_pkg_length(const uint8_t *ptr, const uint8_t *end, uint32_t *out_len, size_t *out_bytes_read) {
    if (ptr >= end) return false;

    uint8_t lead_byte = *ptr;
    uint8_t byte_count = (lead_byte >> 6) & 0x03;

    if (ptr + 1 + byte_count > end) return false;

    if (byte_count == 0) {
        *out_len = lead_byte & 0x3F;
        *out_bytes_read = 1;
        return true;
    }

    uint32_t len = lead_byte & 0x0F;
    for (size_t i = 0; i < byte_count; i++) {
        len |= ((uint32_t)*(ptr + 1 + i)) << (4 + (i * 8));
    }

    *out_len = len;
    *out_bytes_read = 1u + byte_count;
    return true;
}

/**
 * @brief Parses an AML integer element with optional data type prefix.
 * 
 * @param[in,out] ptr_in Double pointer to the current byte stream position (advances on success).
 * @param[in] end Pointer to the end of the available buffer.
 * @param[out] out_val Pointer to store the parsed 16-bit integer value.
 * @return true if the integer was successfully parsed, false otherwise.
 */
static bool aml_parse_integer(const uint8_t **ptr_in, const uint8_t *end, uint16_t *out_val) {
    const uint8_t *ptr = *ptr_in;
    if (ptr >= end) return false;

    uint8_t prefix = *ptr;
    if (prefix == AML_BYTE_PREFIX) {
        if (ptr + 2 > end) return false;
        *out_val = *(ptr + 1);
        *ptr_in = ptr + 2;
        return true;
    } else if (prefix == AML_WORD_PREFIX) {
        if (ptr + 3 > end) return false;
        *out_val = *(uint16_t *)(ptr + 1);
        *ptr_in = ptr + 3;
        return true;
    } else if (prefix == AML_DWORD_PREFIX) {
        if (ptr + 5 > end) return false;
        *out_val = (uint16_t)*(uint32_t *)(ptr + 1);
        *ptr_in = ptr + 5;
        return true;
    }

    *out_val = *ptr;
    *ptr_in = ptr + 1;
    return true;
}

/**
 * @brief Parses the DSDT AML byte stream to find and extract the _S5_ sleep package.
 * 
 * @param[in] dsdt_ptr Pointer to the raw DSDT payload (excluding the SDT header).
 * @param[in] dsdt_length Length of the DSDT payload in bytes.
 * @param[out] out_s5 Pointer to the structure where the extracted S5 sleep values will be stored.
 * @return true if the _S5_ package was found and successfully parsed, false otherwise.
 */
bool aml_parse_s5(const void *dsdt_ptr, size_t dsdt_length, s5_sleep_values_t *out_s5) {
    if (!dsdt_ptr || !out_s5 || dsdt_length == 0) return false;

    memset(out_s5, 0, sizeof(s5_sleep_values_t));

    const uint8_t *ptr = (const uint8_t *)dsdt_ptr;
    const uint8_t *end = ptr + dsdt_length;

    while (ptr < end - 8) {
        if (*ptr == AML_NAME_OP && memcmp(ptr + 1, "_S5_", 4) == 0) {
            ptr += 5;

            if (ptr >= end) break;

            if (*ptr == AML_PACKAGE_OP) {
                ptr++;

                uint32_t pkg_len = 0;
                size_t bytes_read = 0;
                if (!aml_parse_pkg_length(ptr, end, &pkg_len, &bytes_read)) {
                    log_warn("AML: Invalid package length in _S5");
                    return false;
                }
                ptr += bytes_read;

                if (ptr >= end) return false;
                uint8_t elem_count = *ptr++;
                if (elem_count < 2) {
                    log_warn("AML: _S5 package has insufficient elements (%d)", elem_count);
                    return false;
                }

                if (!aml_parse_integer(&ptr, end, &out_s5->slp_typa)) {
                    log_warn("AML: Failed to parse SLP_TYPa");
                    return false;
                }

                if (!aml_parse_integer(&ptr, end, &out_s5->slp_typb)) {
                    log_warn("AML: Failed to parse SLP_TYPb");
                    return false;
                }

                out_s5->valid = true;
                return true;
            }
        }
        ptr++;
    }

    return false;
}
