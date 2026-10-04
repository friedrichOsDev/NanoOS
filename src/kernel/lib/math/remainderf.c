#include <lib/math/libm.h>
#include <lib/math/math.h>

float remainderf(float x, float y) {
    int q;
    return remquof(x, y, &q);
}

weak_alias(remainderf, dremf);
