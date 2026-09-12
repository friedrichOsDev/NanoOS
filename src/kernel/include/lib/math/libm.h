#ifndef _LIBM_H
#define _LIBM_H

#include <float.h>
#include <lib/math/math.h>
#include <stdint.h>

// Musl Internal attribute symbol override
#define hidden

// Hardware representation for x86_64 (Standard 80-bit Extended Precision /
// Little Endian)
union ldshape {
    long double f;
    struct {
        uint64_t m;
        uint16_t se;
    } i;
};

#define WANT_ROUNDING 1
#define WANT_SNAN 0

#define issignalingf_inline(x) 0
#define issignaling_inline(x) 0

#ifdef __GNUC__
#define predict_true(x) __builtin_expect(!!(x), 1)
#define predict_false(x) __builtin_expect(x, 0)
#else
#define predict_true(x) (x)
#define predict_false(x) (x)
#endif

/* Endianness definitions for freestanding x86_64 */
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN

#ifndef weak_alias
#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((weak, alias(#old)))
#endif

static inline float eval_as_float(float x) {
    float y = x;
    return y;
}

static inline double eval_as_double(double x) {
    double y = x;
    return y;
}

#ifndef fp_barrierf
#define fp_barrierf fp_barrierf
static inline float fp_barrierf(float x) {
    volatile float y = x;
    return y;
}
#endif

#ifndef fp_barrier
#define fp_barrier fp_barrier
static inline double fp_barrier(double x) {
    volatile double y = x;
    return y;
}
#endif

#ifndef fp_barrierl
#define fp_barrierl fp_barrierl
static inline long double fp_barrierl(long double x) {
    volatile long double y = x;
    return y;
}
#endif

#ifndef fp_force_evalf
#define fp_force_evalf fp_force_evalf
static inline void fp_force_evalf(float x) {
    volatile float y;
    y = x;
    (void)y;
}
#endif

#ifndef fp_force_eval
#define fp_force_eval fp_force_eval
static inline void fp_force_eval(double x) {
    volatile double y;
    y = x;
    (void)y;
}
#endif

#ifndef fp_force_evall
#define fp_force_evall fp_force_evall
static inline void fp_force_evall(long double x) {
    volatile long double y;
    y = x;
    (void)y;
}
#endif

#define FORCE_EVAL(x)                             \
    do {                                          \
        if (sizeof(x) == sizeof(float)) {         \
            fp_force_evalf(x);                    \
        } else if (sizeof(x) == sizeof(double)) { \
            fp_force_eval(x);                     \
        } else {                                  \
            fp_force_evall(x);                    \
        }                                         \
    } while (0)

#define asuint(f)    \
    ((union {        \
        float _f;    \
        uint32_t _i; \
    }){f})           \
        ._i
#define asfloat(i)   \
    ((union {        \
        uint32_t _i; \
        float _f;    \
    }){i})           \
        ._f
#define asuint64(f)  \
    ((union {        \
        double _f;   \
        uint64_t _i; \
    }){f})           \
        ._i
#define asdouble(i)  \
    ((union {        \
        uint64_t _i; \
        double _f;   \
    }){i})           \
        ._f

#define EXTRACT_WORDS(hi, lo, d)    \
    do {                            \
        uint64_t __u = asuint64(d); \
        (hi) = __u >> 32;           \
        (lo) = (uint32_t)__u;       \
    } while (0)

#define GET_HIGH_WORD(hi, d)      \
    do {                          \
        (hi) = asuint64(d) >> 32; \
    } while (0)

#define GET_LOW_WORD(lo, d)           \
    do {                              \
        (lo) = (uint32_t)asuint64(d); \
    } while (0)

#define INSERT_WORDS(d, hi, lo)                                  \
    do {                                                         \
        (d) = asdouble(((uint64_t)(hi) << 32) | (uint32_t)(lo)); \
    } while (0)

#define SET_HIGH_WORD(d, hi) INSERT_WORDS(d, hi, (uint32_t)asuint64(d))

#define SET_LOW_WORD(d, lo) INSERT_WORDS(d, asuint64(d) >> 32, lo)

#define GET_FLOAT_WORD(w, d) \
    do {                     \
        (w) = asuint(d);     \
    } while (0)

#define SET_FLOAT_WORD(d, w) \
    do {                     \
        (d) = asfloat(w);    \
    } while (0)

/* Internal libm functions */
hidden int __rem_pio2_large(double *, double *, int, int, int);
hidden int __rem_pio2(double, double *);
hidden double __sin(double, double, int);
hidden double __cos(double, double);
hidden double __tan(double, double, int);
hidden double __expo2(double, double);

hidden int __rem_pio2f(float, double *);
hidden float __sindf(double);
hidden float __cosdf(double);
hidden float __tandf(double, int);
hidden float __expo2f(float, float);

hidden int __rem_pio2l(long double, long double *);
hidden long double __sinl(long double, long double, int);
hidden long double __cosl(long double, long double);
hidden long double __tanl(long double, long double, int);

hidden long double __polevll(long double, const long double *, int);
hidden long double __p1evll(long double, const long double *, int);

hidden double __lgamma_r(double, int *);
hidden float __lgammaf_r(float, int *);

/* Error handling stubs */
hidden float __math_xflowf(uint32_t, float);
hidden float __math_uflowf(uint32_t);
hidden float __math_oflowf(uint32_t);
hidden float __math_divzerof(uint32_t);
hidden float __math_invalidf(float);
hidden double __math_xflow(uint32_t, double);
hidden double __math_uflow(uint32_t);
hidden double __math_oflow(uint32_t);
hidden double __math_divzero(uint32_t);
hidden double __math_invalid(double);
hidden long double __math_invalidl(long double);

#endif