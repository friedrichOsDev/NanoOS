#define _GNU_SOURCE
#include <lib/math/math.h>

int finite(double x) { return isfinite(x); }
