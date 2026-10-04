/**
 * @file math_stress_test.c
 * @brief Aggressive FPU, SSE & Libm Math Stress Test Suite for NanoOS.
 * @details Validates trigonometric, exponential, power, rounding functions,
 *          special floating-point values (NaN, Inf), and verifies x87/SSE
 *          register preservation across heavy context switches.
 * @author friedrichOsDev
 */

#include <lib/math/libm.h>
#include <lib/math/math.h>
#include <core/scheduler.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/stress_test.h>
#include <stdbool.h>
#include <stdint.h>

#define MATH_TEST_ASSERT(cond, msg)                                             \
    do {                                                                         \
        if (!(cond)) {                                                           \
            serial_printf(COM1, "[FAIL] Math Stress: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return false;                                                        \
        }                                                                        \
    } while (0)

#define MATH_TEST_PASS(msg) serial_printf(COM1, "[PASS] Math Stress: %s\n", msg)

/* Epsilon tolerance for float and double comparisons */
#define EPSILON_DBL 1e-9
#define EPSILON_FLT 1e-4f

static inline bool dbl_near(double a, double b, double eps) {
    return fabs(a - b) < eps;
}

static inline bool flt_near(float a, float b, float eps) {
    return fabsf(a - b) < eps;
}

/* State variables for concurrent threading tests */
static volatile bool math_fpu_thread_corrupted = false;
static volatile uint64_t math_stress_iterations = 0;

/* =========================================================================
 * Phase 1: Basic Arithmetic & Rounding Operations
 * ========================================================================= */

static bool test_rounding_and_abs() {
    for (int i = 0; i < 10000; i++) {
        MATH_TEST_ASSERT(fabs(-123.456) == 123.456, "fabs failed");
        MATH_TEST_ASSERT(fabsf(-45.67f) == 45.67f, "fabsf failed");

        MATH_TEST_ASSERT(floor(3.7) == 3.0, "floor positive failed");
        MATH_TEST_ASSERT(floor(-3.2) == -4.0, "floor negative failed");
        MATH_TEST_ASSERT(ceil(3.2) == 4.0, "ceil positive failed");
        MATH_TEST_ASSERT(ceil(-3.7) == -3.0, "ceil negative failed");

        MATH_TEST_ASSERT(trunc(3.8) == 3.0, "trunc positive failed");
        MATH_TEST_ASSERT(trunc(-3.8) == -3.0, "trunc negative failed");
        MATH_TEST_ASSERT(round(3.5) == 4.0, "round up failed");
        MATH_TEST_ASSERT(round(-3.5) == -4.0, "round down failed");

        MATH_TEST_ASSERT(fmin(2.5, 4.2) == 2.5, "fmin failed");
        MATH_TEST_ASSERT(fmax(2.5, 4.2) == 4.2, "fmax failed");
    }
    MATH_TEST_PASS("Rounding, Min/Max & Absolute Value Ops");
    return true;
}

/* =========================================================================
 * Phase 2: Trigonometric & Hyperbolic Stress Tests
 * ========================================================================= */

static bool test_trigonometry(void) {
    for (int i = 0; i < 5000; i++) {
        /* Fundamental identities: sin^2(x) + cos^2(x) == 1 */
        double angle = (double)(i % 360) * (M_PI / 180.0);
        double s = sin(angle);
        double c = cos(angle);
        double sq_sum = (s * s) + (c * c);
        MATH_TEST_ASSERT(dbl_near(sq_sum, 1.0, EPSILON_DBL), "sin^2 + cos^2 != 1");

        /* sincos check */
        double s_out, c_out;
        sincos(angle, &s_out, &c_out);
        MATH_TEST_ASSERT(dbl_near(s, s_out, EPSILON_DBL), "sincos sin mismatch");
        MATH_TEST_ASSERT(dbl_near(c, c_out, EPSILON_DBL), "sincos cos mismatch");

        /* Inverse trig (Hauptwertebereich [0, PI/2]) */
        if (angle >= 0.0 && angle <= M_PI_2) {
            MATH_TEST_ASSERT(dbl_near(asin(sin(angle)), angle, 1e-6), "asin(sin(x)) mismatch");
            MATH_TEST_ASSERT(dbl_near(acos(cos(angle)), angle, 1e-6), "acos(cos(x)) mismatch");
        }

        /* Tan / Atan (Hauptwertebereich (-PI/2, PI/2) einhalten) */
        double half_angle = angle / 2.0;
        if (half_angle < M_PI_2 - 1e-4) {
            double t = tan(half_angle);
            MATH_TEST_ASSERT(dbl_near(atan(t), half_angle, 1e-6), "atan(tan(x)) mismatch");
        }

        /* Atan2 (Winkel auf (-PI, PI] abbilden) */
        double expected_atan2 = (angle > M_PI) ? (angle - 2.0 * M_PI) : angle;
        MATH_TEST_ASSERT(dbl_near(atan2(s, c), expected_atan2, 1e-6), "atan2(y, x) mismatch");

        /* Hyperbolic: ch^2(x) - sh^2(x) == 1 */
        double x = (double)(i % 100) / 20.0;
        double sh = sinh(x);
        double ch = cosh(x);
        MATH_TEST_ASSERT(dbl_near((ch * ch) - (sh * sh), 1.0, 1e-5), "cosh^2 - sinh^2 != 1");
    }
    MATH_TEST_PASS("Trigonometric & Hyperbolic Identities");
    return true;
}

/* =========================================================================
 * Phase 3: Exponential, Logarithm & Power Functions
 * ========================================================================= */

static bool test_exp_log_pow() {
    for (int i = 1; i <= 5000; i++) {
        double val = (double)i * 0.1;

        /* exp(log(x)) == x */
        MATH_TEST_ASSERT(dbl_near(exp(log(val)), val, 1e-6), "exp(log(x)) != x");
        MATH_TEST_ASSERT(dbl_near(exp2(log2(val)), val, 1e-6), "exp2(log2(x)) != x");
        MATH_TEST_ASSERT(dbl_near(log10(val), log(val) / M_LN10, 1e-6), "log10 conversion mismatch");

        /* Power and Roots */
        MATH_TEST_ASSERT(dbl_near(pow(val, 2.0), val * val, 1e-5), "pow(x, 2) mismatch");
        MATH_TEST_ASSERT(dbl_near(sqrt(val * val), val, 1e-6), "sqrt(x^2) != x");
        MATH_TEST_ASSERT(dbl_near(cbrt(val * val * val), val, 1e-5), "cbrt(x^3) != x");

        /* Hypot */
        MATH_TEST_ASSERT(dbl_near(hypot(3.0 * val, 4.0 * val), 5.0 * val, 1e-5), "hypot 3-4-5 triangle failed");
    }
    MATH_TEST_PASS("Exponential, Logarithmic & Power Functions");
    return true;
}

/* =========================================================================
 * Phase 4: Edge Cases, Special Values & Floating Point Classification
 * ========================================================================= */

static bool test_fp_edge_cases() {
    /* NaN Handling */
    double nan_val = NAN;
    MATH_TEST_ASSERT(isnan(nan_val), "isnan failed for NAN");
    MATH_TEST_ASSERT(!isfinite(nan_val), "isfinite failed for NAN");
    MATH_TEST_ASSERT(fpclassify(nan_val) == FP_NAN, "fpclassify NAN failed");

    /* Infinity Handling */
    double inf_val = INFINITY;
    MATH_TEST_ASSERT(isinf(inf_val), "isinf failed for INFINITY");
    MATH_TEST_ASSERT(!isfinite(inf_val), "isfinite failed for INFINITY");
    MATH_TEST_ASSERT(fpclassify(inf_val) == FP_INFINITE, "fpclassify INFINITY failed");

    /* Signbit & Copysign */
    MATH_TEST_ASSERT(signbit(-0.0), "signbit failed for -0.0");
    MATH_TEST_ASSERT(!signbit(0.0), "signbit failed for +0.0");
    MATH_TEST_ASSERT(copysign(10.0, -1.0) == -10.0, "copysign failed");

    /* Division by zero / domain errors */
    MATH_TEST_ASSERT(isnan(sqrt(-1.0)), "sqrt(-1) must return NaN");
    MATH_TEST_ASSERT(isinf(log(0.0)), "log(0) must return -Infinity");

    MATH_TEST_PASS("Edge Cases, Special Values (NaN/Inf) & FP Classify");
    return true;
}

/* =========================================================================
 * Phase 5: Heavy SSE & FPU Context Switch Threading Stress
 * ========================================================================= */

/**
 * @brief Thread running heavy x87 FPU stack calculations.
 */
static void math_fpu_stress_worker(void *arg) {
    (void)arg;
    for (int loop = 0; loop < 200; loop++) {
        double acc = 1.0;
        for (int i = 1; i <= 500; i++) {
            acc += sin((double)i) * cos((double)i);
            acc = sqrt(fabs(acc));
        }

        /* Check calculation sanity */
        if (isnan(acc) || isinf(acc)) {
            math_fpu_thread_corrupted = true;
        }
        math_stress_iterations++;
        thread_yield();
    }
    thread_exit();
}

/**
 * @brief Thread running heavy SSE float / vector-style calculations.
 */
static void math_sse_stress_worker(void *arg) {
    (void)arg;
    for (int loop = 0; loop < 200; loop++) {
        float f1 = 1.2345f;
        float f2 = 6.7890f;
        for (int i = 0; i < 500; i++) {
            f1 = fabsf(f1 * 1.0001f + fmaf(f1, f2, 0.5f));
            f2 = sqrtf(f1 + 1.0f);
        }

        if (isnan(f1) || isnan(f2)) {
            math_fpu_thread_corrupted = true;
        }
        math_stress_iterations++;
        thread_yield();
    }
    thread_exit();
}

static bool test_concurrent_fpu_sse_stress() {
    math_fpu_thread_corrupted = false;
    math_stress_iterations = 0;

    /* Spawn parallel worker threads using x87 FPU and SSE registers */
    for (int i = 0; i < 4; i++) {
        thread_create(NULL, math_fpu_stress_worker, NULL, "fpu_stress");
        thread_create(NULL, math_sse_stress_worker, NULL, "sse_stress");
    }

    thread_sleep_ms(800);

    MATH_TEST_ASSERT(!math_fpu_thread_corrupted, "FPU/SSE state corrupted during context switches");
    MATH_TEST_ASSERT(math_stress_iterations > 0, "Stress threads did not execute");

    serial_printf(COM1, "[INFO] Math Stress: Executed %lu concurrent math iterations\n", math_stress_iterations);
    MATH_TEST_PASS("Concurrent Multi-Threaded FPU & SSE Stress");
    return true;
}

/* =========================================================================
 * Entry Point for Math Stress Suite
 * ========================================================================= */

bool run_math_lib_stress_test() {
    serial_printf(COM1, "\n--- [Math Lib & FPU/SSE Aggressive Stress Test] ---\n");

    bool success = true;
    success &= test_rounding_and_abs();
    success &= test_trigonometry();
    success &= test_exp_log_pow();
    success &= test_fp_edge_cases();
    success &= test_concurrent_fpu_sse_stress();

    if (success) {
        serial_printf(COM1, "[SUCCESS] All Math & FPU/SSE Stress Tests Passed Successfully!\n");
    } else {
        serial_printf(COM1, "[FAILURE] Math & FPU/SSE Stress Tests Failed!\n");
    }
    return success;
}