#include <float.h>
#include <lib/math/math.h>
#include <limits.h>

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double scalblnl(long double x, long n) { return scalbln(x, n); }
#else
long double scalblnl(long double x, long n) {
    if (n > INT_MAX)
        n = INT_MAX;
    else if (n < INT_MIN)
        n = INT_MIN;
    return scalbnl(x, n);
}
#endif
