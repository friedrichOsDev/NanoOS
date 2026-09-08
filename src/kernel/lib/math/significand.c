#define _GNU_SOURCE
#include <lib/math/math.h>

double significand(double x) { return scalbn(x, -ilogb(x)); }
