#include "libm.h"
#include <pch.h>

float lgammaf(float x) { return __lgammaf_r(x, &__signgam); }
