#include "libm.h"
#include <lib/math.h>

double lgamma(double x) { return __lgamma_r(x, &__signgam); }
