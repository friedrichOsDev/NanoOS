#include <lib/math/libm.h>
#include <lib/math/math.h>

double lgamma(double x) { return __lgamma_r(x, &__signgam); }
