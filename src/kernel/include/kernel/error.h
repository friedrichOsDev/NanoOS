/**
 * @file error.h
 * @brief Error handling.
 * @author friedrichOsDev
 */

#pragma once

typedef enum {
    E_OK = 0,
    E_TIMEOUT = -1,
    E_INVAL = -2,
    E_CHECKSUM = -3,
    E_NOMEM = -4,
    E_BADADDR = -5,
    E_NOTFOUND = -6,
    E_NOINIT = -7,
    E_NOPROBE = -8,
    E_BADCOUNT = -9,
    E_BADSIZE = -10,
    E_NOTAVAIL = -11,
    // E_...
} kerr_t;

#include <lib/log.h>

static inline const char *kerr_str(kerr_t e) {
    switch (e) {
    case E_OK:
        return "E_OK";
    case E_TIMEOUT:
        return "E_TIMEOUT";
    case E_INVAL:
        return "E_INVAL";
    case E_CHECKSUM:
        return "E_CHECKSUM";
    case E_NOMEM:
        return "E_NOMEM";
    case E_BADADDR:
        return "E_BADADDR";
    case E_NOTFOUND:
        return "E_NOTFOUND";
    case E_NOINIT:
        return "E_NOINIT";
    case E_NOPROBE:
        return "E_NOPROBE";
    case E_BADCOUNT:
        return "E_BADCOUNT";
    case E_BADSIZE:
        return "E_BADSIZE";
    case E_NOTAVAIL:
        return "E_NOTAVAIL";
    // E_...
    default:
        return "E_UNKNOWN";
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define k_failed(e) (e < 0)
#define k_assert(cond)                                                                      \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            log_assert("\"%s\" failed (%s:%d)", #cond, __builtin_FILE(), __builtin_LINE()); \
        }                                                                                   \
    } while (0)

#define K_TRY(expr)           \
    do {                      \
        kerr_t _err = (expr); \
        if (k_failed(_err)) { \
            return _err;      \
        }                     \
    } while (0)

#define K_TRY_PANIC(expr) \
    do { \
        kerr_t _err = (expr); \
        if (k_failed(_err)) { \
            log_panic("\"%s\" failed with error \"%s\" (%s:%d)", #expr, kerr_str(_err), __builtin_FILE(), __builtin_LINE()); \
        } \
    } while (0)

#pragma GCC diagnostic pop
