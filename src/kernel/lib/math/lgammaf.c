#include <lib/math/libm.h>
#include <lib/math/math.h>

float lgammaf(float x) { return __lgammaf_r(x, &__signgam); }
