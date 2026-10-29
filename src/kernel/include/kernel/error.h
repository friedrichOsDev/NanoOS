#pragma once

#include <arch/arch.h>
#include <kernel/panic.h>
#include <pch.h>

typedef enum {
    E_OK = 0,
    E_TIMEOUT = -1,
    // E_...
} kerr_t;
// TODO: add kerr_t to every faulting function
// TODO: add more E_...

inline const char *kerr_str(kerr_t e) {
    switch (e) {
    case E_OK:
        return "E_OK";
    case E_TIMEOUT:
        return "E_TIMEOUT";
    // E_...
    default:
        return "E_UNKNOWN";
    }
}

#define k_failed(e) (e < 0)
#define k_assert(cond)                                                                      \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            log_assert("\"%s\" failed (%s:%d)", #cond, __builtin_FILE(), __builtin_LINE()); \
        }                                                                                   \
    } while (0)
