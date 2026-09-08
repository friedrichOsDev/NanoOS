#include <lib/math/math.h>
#include <lib/math/libm.h>

double remainder(double x, double y) {
    int q;
    return remquo(x, y, &q);
}

weak_alias(remainder, drem);
