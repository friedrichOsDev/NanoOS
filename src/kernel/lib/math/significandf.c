#define _GNU_SOURCE
#include <lib/math/math.h>

float significandf(float x) { return scalbnf(x, -ilogbf(x)); }
