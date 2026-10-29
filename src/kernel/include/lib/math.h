#pragma once

#include <pch.h>

/* Floating-point constants */
#undef NAN
#define NAN __builtin_nanf("")
#undef INFINITY
#define INFINITY __builtin_inff()

#define HUGE_VALF INFINITY
#define HUGE_VAL ((double)INFINITY)
#define HUGE_VALL ((long double)INFINITY)

/* Classification constants */
#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4
#define FP_ILOGB0 (-INT_MAX)
#define FP_ILOGBNAN INT_MAX

/* Useful Math Constants */
#define M_E 2.7182818284590452354
#define M_LOG2E 1.4426950408889634074
#define M_LOG10E 0.43429448190325182765
#define M_LN2 0.69314718055994530942
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962
#define M_1_PI 0.31830988618379067154
#define M_2_PI 0.63661977236758134308
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2 1.41421356237309504880
#define M_SQRT1_2 0.70710678118654752440

static inline float deg_to_radf(float deg) { return deg * ((float)M_PI / 180.0f); }
static inline double deg_to_rad(double deg) { return deg * (M_PI / 180.0); }
static inline long double deg_to_radl(long double deg) { return deg * (M_PI / 180.0L); }

static inline float rad_to_degf(float rad) { return rad * (180.0f / (float)M_PI); }
static inline double rad_to_deg(double rad) { return rad * (180.0 / M_PI); }
static inline long double rad_to_degl(long double rad) { return rad * (180.0L / M_PI); }

/* Standard C99 / C11 Floating-Point Types for x86_64 */
typedef float float_t;
typedef double double_t;

extern int __signgam;
#define signgam __signgam

/* Builtin Bithacks */
static inline unsigned __FLOAT_BITS(float __f) {
    union {
        float __f;
        unsigned __i;
    } __u;
    __u.__f = __f;
    return __u.__i;
}

static inline unsigned long long __DOUBLE_BITS(double __f) {
    union {
        double __f;
        unsigned long long __i;
    } __u;
    __u.__f = __f;
    return __u.__i;
}

int __fpclassify(double);
int __fpclassifyf(float);
int __fpclassifyl(long double);

#define fpclassify(x) (                                                                           \
    sizeof(x) == sizeof(float) ? __fpclassifyf(x) : sizeof(x) == sizeof(double) ? __fpclassify(x) \
                                                                                : __fpclassifyl(x))

#define isinf(x) (                                                                                                                                              \
    sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) == 0x7f800000 : sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL >> 1) == 0x7ffULL << 52 \
                                                                                                            : __fpclassifyl(x) == FP_INFINITE)

#define isnan(x) (                                                                                                                                            \
    sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) > 0x7f800000 : sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL >> 1) > 0x7ffULL << 52 \
                                                                                                           : __fpclassifyl(x) == FP_NAN)

#define isnormal(x) (                                                                                                                                                                       \
    sizeof(x) == sizeof(float) ? ((__FLOAT_BITS(x) + 0x00800000) & 0x7fffffff) >= 0x01000000 : sizeof(x) == sizeof(double) ? ((__DOUBLE_BITS(x) + (1ULL << 52)) & -1ULL >> 1) >= 1ULL << 53 \
                                                                                                                           : __fpclassifyl(x) == FP_NORMAL)

#define isfinite(x) (                                                                                                                                         \
    sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) < 0x7f800000 : sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL >> 1) < 0x7ffULL << 52 \
                                                                                                           : __fpclassifyl(x) > FP_INFINITE)

int __signbit(double);
int __signbitf(float);
int __signbitl(long double);

#define signbit(x) (                                                                                                        \
    sizeof(x) == sizeof(float) ? (int)(__FLOAT_BITS(x) >> 31) : sizeof(x) == sizeof(double) ? (int)(__DOUBLE_BITS(x) >> 63) \
                                                                                            : __signbitl(x))

/* Trigonometric functions */
double acos(double);
float acosf(float);
long double acosl(long double);

double asin(double);
float asinf(float);
long double asinl(long double);

double atan(double);
float atanf(float);
long double atanl(long double);

double atan2(double, double);
float atan2f(float, float);
long double atan2l(long double, long double);

double cos(double);
float cosf(float);
long double cosl(long double);

double sin(double);
float sinf(float);
long double sinl(long double);

double tan(double);
float tanf(float);
long double tanl(long double);

void sincos(double, double *, double *);
void sincosf(float, float *, float *);
void sincosl(long double, long double *, long double *);

/* Hyperbolic functions */
double acosh(double);
float acoshf(float);
long double acoshl(long double);

double asinh(double);
float asinhf(float);
long double asinhl(long double);

double atanh(double);
float atanhf(float);
long double atanhl(long double);

double cosh(double);
float coshf(float);
long double coshl(long double);

double sinh(double);
float sinhf(float);
long double sinhl(long double);

double tanh(double);
float tanhf(float);
long double tanhl(long double);

/* Exponential and logarithmic functions */
double exp(double);
float expf(float);
long double expl(long double);

double exp2(double);
float exp2f(float);
long double exp2l(long double);

double exp10(double);
float exp10f(float);
long double exp10l(long double);

double expm1(double);
float expm1f(float);
long double expm1l(long double);

double log(double);
float logf(float);
long double logl(long double);

double log1p(double);
float log1pf(float);
long double log1pl(long double);

double log10(double);
float log10f(float);
long double log10l(long double);

double log2(double);
float log2f(float);
long double log2l(long double);

double logb(double);
float logbf(float);
long double logbl(long double);

int ilogb(double);
int ilogbf(float);
int ilogbl(long double);

/* Power and absolute value functions */
double pow(double, double);
float powf(float, float);
long double powl(long double, long double);

double sqrt(double);
float sqrtf(float);
long double sqrtl(long double);

double cbrt(double);
float cbrtf(float);
long double cbrtl(long double);

double hypot(double, double);
float hypotf(float, float);
long double hypotl(long double, long double);

double fabs(double);
float fabsf(float);
long double fabsl(long double);

/* Nearest integer floating-point operations */
double ceil(double);
float ceilf(float);
long double ceill(long double);

double floor(double);
float floorf(float);
long double floorl(long double);

double trunc(double);
float truncf(float);
long double truncl(long double);

double round(double);
float roundf(float);
long double roundl(long double);

long lround(double);
long lroundf(float);
long lroundl(long double);

long long llround(double);
long long llroundf(float);
long long llroundl(long double);

double rint(double);
float rintf(float);
long double rintl(long double);

long lrint(double);
long lrintf(float);
long lrintl(long double);

long long llrint(double);
long long llrintf(float);
long long llrintl(long double);

double nearbyint(double);
float nearbyintf(float);
long double nearbyintl(long double);

/* Modulo, division and floating-point manipulation */
double fmod(double, double);
float fmodf(float, float);
long double fmodl(long double, long double);

double modf(double, double *);
float modff(float, float *);
long double modfl(long double, long double *);

double remainder(double, double);
float remainderf(float, float);
long double remainderl(long double, long double);

double remquo(double, double, int *);
float remquof(float, float, int *);
long double remquol(long double, long double, int *);

double copysign(double, double);
float copysignf(float, float);
long double copysignl(long double, long double);

double nan(const char *);
float nanf(const char *);
long double nanl(const char *);

double nextafter(double, double);
float nextafterf(float, float);
long double nextafterl(long double, long double);

double nexttoward(double, long double);
float nexttowardf(float, long double);
long double nexttowardl(long double, long double);

double frexp(double, int *);
float frexpf(float, int *);
long double frexpl(long double, int *);

double ldexp(double, int);
float ldexpf(float, int);
long double ldexpl(long double, int);

double scalbn(double, int);
float scalbnf(float, int);
long double scalbnl(long double, int);

double scalb(double, double);
float scalbf(float, float);

double scalbln(double, long);
float scalblnf(float, long);
long double scalblnl(long double, long);

/* Max, min and difference */
double fmax(double, double);
float fmaxf(float, float);
long double fmaxl(long double, long double);

double fmin(double, double);
float fminf(float, float);
long double fminl(long double, long double);

double fdim(double, double);
float fdimf(float, float);
long double fdiml(long double, long double);

double fma(double, double, double);
float fmaf(float, float, float);
long double fmal(long double, long double, long double);

/* Special functions */
double erf(double);
float erff(float);
long double erfl(long double);
double erfc(double);
float erfcf(float);
long double erfcl(long double);

double lgamma(double);
float lgammaf(float);
long double lgammal(long double);

double lgamma_r(double, int *);
float lgammaf_r(float, int *);

double tgamma(double);
float tgammaf(float);
long double tgammal(long double);

/* Bessel functions */
double j0(double);
double j1(double);
double jn(int, double);

float j0f(float);
float j1f(float);
float jnf(int, float);

double y0(double);
double y1(double);
double yn(int, double);

float y0f(float);
float y1f(float);
float ynf(int, float);

/* Legacy & extra helper functions */
int finite(double);
int finitef(float);

double significand(double);
float significandf(float);

/* Hardware Builtin Overrides for x86_64 */
#define fabs(x) __builtin_fabs(x)
#define fabsf(x) __builtin_fabsf(x)
#define sqrt(x) __builtin_sqrt(x)
#define sqrtf(x) __builtin_sqrtf(x)

// GENERIC API

#define MATH_1ARG(fn, x) _Generic((x), \
    float: fn##f(x),                   \
    double: fn(x),                     \
    long double: fn##l(x))

#define MATH_2ARG(fn, x, y) _Generic((x) + (y), \
    float: fn##f(x, y),                         \
    double: fn(x, y),                           \
    long double: fn##l(x, y))

#define math_acos(x) MATH_1ARG(acos, x)
#define math_asin(x) MATH_1ARG(asin, x)
#define math_atan(x) MATH_1ARG(atan, x)
#define math_atan2(y, x) MATH_2ARG(atan2, y, x)
#define math_cos(x) MATH_1ARG(cos, x)
#define math_sin(x) MATH_1ARG(sin, x)
#define math_tan(x) MATH_1ARG(tan, x)
#define math_sincos(x, s, c) _Generic((x),           \
    float: sincosf(x, (float *)(s), (float *)(c)),   \
    double: sincos(x, (double *)(s), (double *)(c)), \
    long double: sincosl(x, (long double *)(s), (long double *)(c)))
#define math_acosh(x) MATH_1ARG(acosh, x)
#define math_asinh(x) MATH_1ARG(asinh, x)
#define math_atanh(x) MATH_1ARG(atanh, x)
#define math_cosh(x) MATH_1ARG(cosh, x)
#define math_sinh(x) MATH_1ARG(sinh, x)
#define math_tanh(x) MATH_1ARG(tanh, x)
#define math_exp(x) MATH_1ARG(exp, x)
#define math_exp2(x) MATH_1ARG(exp2, x)
#define math_exp10(x) MATH_1ARG(exp10, x)
#define math_expm1(x) MATH_1ARG(expm1, x)
#define math_log(x) MATH_1ARG(log, x)
#define math_log1p(x) MATH_1ARG(log1p, x)
#define math_log10(x) MATH_1ARG(log10, x)
#define math_log2(x) MATH_1ARG(log2, x)
#define math_logb(x) MATH_1ARG(logb, x)
#define math_pow(base, exp) MATH_2ARG(pow, base, exp)
#define math_sqrt(x) MATH_1ARG(sqrt, x)
#define math_cbrt(x) MATH_1ARG(cbrt, x)
#define math_hypot(x, y) MATH_2ARG(hypot, x, y)
#define math_fabs(x) MATH_1ARG(fabs, x)
#define math_ceil(x) MATH_1ARG(ceil, x)
#define math_floor(x) MATH_1ARG(floor, x)
#define math_trunc(x) MATH_1ARG(trunc, x)
#define math_round(x) MATH_1ARG(round, x)
#define math_rint(x) MATH_1ARG(rint, x)
#define math_nearbyint(x) MATH_1ARG(nearbyint, x)
#define math_fmod(x, y) MATH_2ARG(fmod, x, y)
#define math_remainder(x, y) MATH_2ARG(remainder, x, y)
#define math_copysign(x, y) MATH_2ARG(copysign, x, y)
#define math_nextafter(x, y) MATH_2ARG(nextafter, x, y)
#define math_fmax(x, y) MATH_2ARG(fmax, x, y)
#define math_fmin(x, y) MATH_2ARG(fmin, x, y)
#define math_fdim(x, y) MATH_2ARG(fdim, x, y)
#define math_fma(x, y, z) _Generic((x), \
    float: fmaf(x, y, z),               \
    double: fma(x, y, z),               \
    long double: fmal(x, y, z))
#define math_erf(x) MATH_1ARG(erf, x)
#define math_erfc(x) MATH_1ARG(erfc, x)
#define math_lgamma(x) MATH_1ARG(lgamma, x)
#define math_tgamma(x) MATH_1ARG(tgamma, x)
#define math_deg_to_rad(x) MATH_1ARG(deg_to_rad, x)
#define math_rad_to_deg(x) MATH_1ARG(rad_to_deg, x)
