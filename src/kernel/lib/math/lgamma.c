#include "libm.h"
#include <pch.h>

double lgamma(double x) { return __lgamma_r(x, &__signgam); }
