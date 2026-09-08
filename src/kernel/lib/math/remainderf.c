#include <lib/math/math.h>
#include <lib/math/libm.h>

float remainderf(float x, float y) {
    int q;
    return remquof(x, y, &q);
}

weak_alias(remainderf, dremf);
