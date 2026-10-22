#include "libm.h"
#include <lib/math.h>

float lgammaf(float x) { return __lgammaf_r(x, &__signgam); }
