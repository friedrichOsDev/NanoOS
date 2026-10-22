#include "libm.h"
#include <lib/math.h>

float remainderf(float x, float y) {
    int q;
    return remquof(x, y, &q);
}

weak_alias(remainderf, dremf);
